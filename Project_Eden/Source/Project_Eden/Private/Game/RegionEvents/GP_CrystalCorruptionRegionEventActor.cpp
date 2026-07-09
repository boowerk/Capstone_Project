#include "Game/RegionEvents/GP_CrystalCorruptionRegionEventActor.h"

#include "Characters/GP_PlayerCharacter.h"
#include "Game/RegionEvents/GP_RegionEventCrystalNode.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"

AGP_CrystalCorruptionRegionEventActor::AGP_CrystalCorruptionRegionEventActor()
{
	CrystalNodeClass = AGP_RegionEventCrystalNode::StaticClass();
}

void AGP_CrystalCorruptionRegionEventActor::ActivateRegionEvent()
{
	if (GetRuntimeState() != EGPRegionEventRuntimeState::Dormant)
	{
		return;
	}

	Super::ActivateRegionEvent();

	if (!HasAuthority() || GetRuntimeState() != EGPRegionEventRuntimeState::Active)
	{
		return;
	}

	SpawnCrystalNodes();
	MulticastSetCorruptionSlowActive(true);

	if (ActiveCrystals.IsEmpty())
	{
		CompleteRegionEvent();
	}
}

void AGP_CrystalCorruptionRegionEventActor::CompleteRegionEvent()
{
	MulticastSetCorruptionSlowActive(false);
	DestroyRemainingCrystals();
	Super::CompleteRegionEvent();
}

void AGP_CrystalCorruptionRegionEventActor::ExpireRegionEvent()
{
	MulticastSetCorruptionSlowActive(false);
	DestroyRemainingCrystals();
	Super::ExpireRegionEvent();
}

void AGP_CrystalCorruptionRegionEventActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	MulticastSetCorruptionSlowActive(false);
	DestroyRemainingCrystals();
	Super::EndPlay(EndPlayReason);
}

void AGP_CrystalCorruptionRegionEventActor::SpawnCrystalNodes()
{
	if (!HasAuthority() || !*CrystalNodeClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const int32 SafeCrystalCount = FMath::Max(1, CrystalCount);
	for (int32 CrystalIndex = 0; CrystalIndex < SafeCrystalCount; ++CrystalIndex)
	{
		const float AngleRadians = (2.0f * PI * CrystalIndex) / SafeCrystalCount;
		const FVector RingOffset(
			FMath::Cos(AngleRadians) * CrystalRingRadius,
			FMath::Sin(AngleRadians) * CrystalRingRadius,
			80.0f);
		FVector DesiredLocation = GetActorLocation() + RingOffset;

		if (const UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(World))
		{
			FNavLocation ProjectedLocation;
			if (NavigationSystem->ProjectPointToNavigation(DesiredLocation, ProjectedLocation, FVector(400.0f, 400.0f, 1200.0f)))
			{
				DesiredLocation = ProjectedLocation.Location + FVector(0.0f, 0.0f, 80.0f);
			}
		}

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		AGP_RegionEventCrystalNode* CrystalNode = World->SpawnActor<AGP_RegionEventCrystalNode>(
			CrystalNodeClass,
			DesiredLocation,
			FRotator(0.0f, FMath::RadiansToDegrees(AngleRadians), 0.0f),
			SpawnParameters);
		if (!IsValid(CrystalNode))
		{
			continue;
		}

		CrystalNode->OnCrystalNodeDestroyed.AddDynamic(this, &ThisClass::HandleCrystalDestroyed);
		ActiveCrystals.Add(CrystalNode);
	}
}

void AGP_CrystalCorruptionRegionEventActor::DestroyRemainingCrystals()
{
	if (!HasAuthority())
	{
		return;
	}

	for (AGP_RegionEventCrystalNode* CrystalNode : ActiveCrystals)
	{
		if (IsValid(CrystalNode))
		{
			CrystalNode->Destroy();
		}
	}

	ActiveCrystals.Reset();
}

void AGP_CrystalCorruptionRegionEventActor::RefreshPlayerSlow()
{
	if (!bCorruptionSlowActive)
	{
		RestoreAllSlowedPlayers();
		return;
	}

	TArray<AActor*> PlayerActors;
	UGameplayStatics::GetAllActorsOfClass(this, AGP_PlayerCharacter::StaticClass(), PlayerActors);

	TSet<AGP_PlayerCharacter*> PlayersInsideRadius;
	for (AActor* PlayerActor : PlayerActors)
	{
		AGP_PlayerCharacter* PlayerCharacter = Cast<AGP_PlayerCharacter>(PlayerActor);
		if (!IsValid(PlayerCharacter))
		{
			continue;
		}

		const bool bShouldApplyOnThisMachine = HasAuthority() || PlayerCharacter->IsLocallyControlled();
		if (!bShouldApplyOnThisMachine)
		{
			continue;
		}

		if (FVector::DistSquared2D(PlayerCharacter->GetActorLocation(), GetActorLocation()) > FMath::Square(SlowRadius))
		{
			continue;
		}

		PlayersInsideRadius.Add(PlayerCharacter);
		if (FindSlowedPlayerIndex(PlayerCharacter) == INDEX_NONE)
		{
			FSlowedPlayerRecord Record;
			Record.Player = PlayerCharacter;
			Record.PreviousMultiplier = PlayerCharacter->GetGASMovementSpeedMultiplier();
			SlowedPlayers.Add(Record);
			PlayerCharacter->SetGASMovementSpeedMultiplier(Record.PreviousMultiplier * SlowMultiplier);
		}
	}

	for (int32 Index = SlowedPlayers.Num() - 1; Index >= 0; --Index)
	{
		AGP_PlayerCharacter* PlayerCharacter = SlowedPlayers[Index].Player.Get();
		if (!IsValid(PlayerCharacter) || !PlayersInsideRadius.Contains(PlayerCharacter))
		{
			if (IsValid(PlayerCharacter))
			{
				PlayerCharacter->SetGASMovementSpeedMultiplier(SlowedPlayers[Index].PreviousMultiplier);
			}
			SlowedPlayers.RemoveAtSwap(Index);
		}
	}
}

void AGP_CrystalCorruptionRegionEventActor::RestoreAllSlowedPlayers()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SlowRefreshTimerHandle);
	}

	for (const FSlowedPlayerRecord& Record : SlowedPlayers)
	{
		if (AGP_PlayerCharacter* PlayerCharacter = Record.Player.Get())
		{
			PlayerCharacter->SetGASMovementSpeedMultiplier(Record.PreviousMultiplier);
		}
	}

	SlowedPlayers.Reset();
}

int32 AGP_CrystalCorruptionRegionEventActor::FindSlowedPlayerIndex(const AGP_PlayerCharacter* PlayerCharacter) const
{
	for (int32 Index = 0; Index < SlowedPlayers.Num(); ++Index)
	{
		if (SlowedPlayers[Index].Player.Get() == PlayerCharacter)
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

void AGP_CrystalCorruptionRegionEventActor::HandleCrystalDestroyed(AGP_RegionEventCrystalNode* CrystalNode, AActor* DestroyingActor)
{
	ActiveCrystals.Remove(CrystalNode);

	for (int32 Index = ActiveCrystals.Num() - 1; Index >= 0; --Index)
	{
		if (!IsValid(ActiveCrystals[Index]))
		{
			ActiveCrystals.RemoveAtSwap(Index);
		}
	}

	if (HasAuthority() && ActiveCrystals.IsEmpty())
	{
		CompleteRegionEvent();
	}
}

void AGP_CrystalCorruptionRegionEventActor::MulticastSetCorruptionSlowActive_Implementation(bool bActive)
{
	if (!bActive)
	{
		bCorruptionSlowActive = false;
		RestoreAllSlowedPlayers();
		return;
	}

	bCorruptionSlowActive = true;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			SlowRefreshTimerHandle,
			this,
			&ThisClass::RefreshPlayerSlow,
			FMath::Max(0.05f, SlowRefreshInterval),
			true,
			0.0f);
	}
}
