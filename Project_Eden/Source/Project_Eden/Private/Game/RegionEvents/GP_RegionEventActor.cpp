#include "Game/RegionEvents/GP_RegionEventActor.h"

#include "Characters/GP_EnemyCharacter.h"
#include "Characters/GP_PlayerCharacter.h"
#include "Components/SphereComponent.h"
#include "Game/GP_GameState.h"
#include "Game/RegionEvents/GP_RegionEventData.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"

AGP_RegionEventActor::AGP_RegionEventActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	NetUpdateFrequency = 5.0f;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	ActivationTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("ActivationTrigger"));
	ActivationTrigger->SetupAttachment(SceneRoot);
	ActivationTrigger->SetSphereRadius(ActivationRadius);
	ActivationTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ActivationTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	ActivationTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	ActivationTrigger->SetGenerateOverlapEvents(true);
	ActivationTrigger->ShapeColor = FColor(190, 80, 230);
}

void AGP_RegionEventActor::BeginPlay()
{
	Super::BeginPlay();

	ActivationTrigger->SetSphereRadius(ActivationRadius);
	if (HasAuthority())
	{
		ActivationTrigger->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandleActivationOverlap);
	}
}

void AGP_RegionEventActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoExpireTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void AGP_RegionEventActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGP_RegionEventActor, RegionId);
	DOREPLIFETIME(AGP_RegionEventActor, EventData);
	DOREPLIFETIME(AGP_RegionEventActor, RuntimeState);
}

void AGP_RegionEventActor::InitializeRegionEvent(int32 InRegionId, UGP_RegionEventData* InEventData)
{
	if (!HasAuthority())
	{
		return;
	}

	RegionId = InRegionId;
	EventData = InEventData;
	RuntimeState = EGPRegionEventRuntimeState::Dormant;

	BP_OnRegionEventInitialized();
	OnRegionEventStateChanged.Broadcast(this);
	ForceNetUpdate();
}

void AGP_RegionEventActor::ActivateRegionEvent()
{
	if (!HasAuthority() || RuntimeState != EGPRegionEventRuntimeState::Dormant || !IsValid(EventData))
	{
		return;
	}

	if (EventData->bApplyActiveRegionState)
	{
		ApplyRegionState(EventData->ActiveRegionState);
	}

	SetRuntimeState(EGPRegionEventRuntimeState::Active);
	SpawnConfiguredEnemies();

	// Duration < 0 means Blueprint or director will complete the event explicitly.
	if (EventData->DurationSeconds >= 0.0f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				AutoExpireTimerHandle,
				this,
				&ThisClass::ExpireRegionEvent,
				EventData->DurationSeconds,
				false);
		}
	}
}

void AGP_RegionEventActor::CompleteRegionEvent()
{
	if (!HasAuthority()
		|| RuntimeState == EGPRegionEventRuntimeState::Completed
		|| RuntimeState == EGPRegionEventRuntimeState::Expired)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoExpireTimerHandle);
	}

	if (IsValid(EventData) && EventData->bApplyCompletedRegionState)
	{
		ApplyRegionState(EventData->CompletedRegionState);
	}

	SetRuntimeState(EGPRegionEventRuntimeState::Completed);
}

void AGP_RegionEventActor::ExpireRegionEvent()
{
	if (!HasAuthority()
		|| RuntimeState == EGPRegionEventRuntimeState::Completed
		|| RuntimeState == EGPRegionEventRuntimeState::Expired)
	{
		return;
	}

	SetRuntimeState(EGPRegionEventRuntimeState::Expired);
}

void AGP_RegionEventActor::SetRuntimeState(EGPRegionEventRuntimeState NewState)
{
	if (RuntimeState == NewState)
	{
		return;
	}

	RuntimeState = NewState;
	DispatchStatePresentation(RuntimeState);
	OnRegionEventStateChanged.Broadcast(this);
	ForceNetUpdate();
}

void AGP_RegionEventActor::ApplyRegionState(uint8 NewState) const
{
	if (AGP_GameState* GPGameState = GetWorld() ? GetWorld()->GetGameState<AGP_GameState>() : nullptr)
	{
		// Region visuals already replicate through AGP_GameState, so event actors only write on the server.
		GPGameState->SetRegionState(RegionId, NewState);
	}
}

void AGP_RegionEventActor::SpawnConfiguredEnemies()
{
	if (!HasAuthority() || !IsValid(EventData))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (const FGP_EnemySpawnEntry& SpawnEntry : EventData->EnemySpawns)
	{
		if (!*SpawnEntry.EnemyClass)
		{
			continue;
		}

		for (int32 SpawnIndex = 0; SpawnIndex < SpawnEntry.Count; ++SpawnIndex)
		{
			bool bProjected = false;
			const FVector SpawnLocation = ResolveSpawnLocation(EventData->EnemySpawnScatterRadius, bProjected);
			if (!bProjected)
			{
				UE_LOG(LogTemp, Warning, TEXT("[RegionEvent] Skipped enemy spawn for %s: navmesh projection failed."),
					*GetNameSafe(this));
				continue;
			}

			FActorSpawnParameters SpawnParameters;
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			AGP_EnemyCharacter* SpawnedEnemy = World->SpawnActor<AGP_EnemyCharacter>(
				SpawnEntry.EnemyClass,
				SpawnLocation,
				FRotator::ZeroRotator,
				SpawnParameters);
			if (!IsValid(SpawnedEnemy))
			{
				continue;
			}

			SpawnedEnemy->SpawnDefaultController();
			OnRegionEventEnemySpawned.Broadcast(this, SpawnedEnemy);
		}
	}
}

FVector AGP_RegionEventActor::ResolveSpawnLocation(float ScatterRadius, bool& bOutProjected) const
{
	const float SafeScatterRadius = FMath::Max(0.0f, ScatterRadius);
	const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
	const float Distance = SafeScatterRadius * FMath::Sqrt(FMath::FRand());
	const FVector DesiredLocation = GetActorLocation()
		+ FVector(FMath::Cos(Angle) * Distance, FMath::Sin(Angle) * Distance, SpawnZOffset);

	bOutProjected = false;
	if (const UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld()))
	{
		FNavLocation ProjectedLocation;
		if (NavigationSystem->ProjectPointToNavigation(DesiredLocation, ProjectedLocation, NavProjectionExtent))
		{
			bOutProjected = true;
			return ProjectedLocation.Location;
		}
	}

	return DesiredLocation;
}

void AGP_RegionEventActor::DispatchStatePresentation(EGPRegionEventRuntimeState PresentedState)
{
	switch (PresentedState)
	{
	case EGPRegionEventRuntimeState::Active:
		BP_OnRegionEventActivated();
		break;
	case EGPRegionEventRuntimeState::Completed:
		BP_OnRegionEventCompleted();
		break;
	case EGPRegionEventRuntimeState::Expired:
		BP_OnRegionEventExpired();
		break;
	default:
		break;
	}
}

void AGP_RegionEventActor::OnRep_RuntimeState(EGPRegionEventRuntimeState PreviousState)
{
	if (PreviousState != RuntimeState)
	{
		DispatchStatePresentation(RuntimeState);
		OnRegionEventStateChanged.Broadcast(this);
	}
}

void AGP_RegionEventActor::HandleActivationOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bActivateOnPlayerOverlap || RuntimeState != EGPRegionEventRuntimeState::Dormant || !Cast<AGP_PlayerCharacter>(OtherActor))
	{
		return;
	}

	ActivateRegionEvent();
}
