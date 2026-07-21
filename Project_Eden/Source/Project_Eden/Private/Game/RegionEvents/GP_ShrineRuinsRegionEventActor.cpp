#include "Game/RegionEvents/GP_ShrineRuinsRegionEventActor.h"

#include "Characters/GP_PlayerCharacter.h"
#include "Components/SphereComponent.h"
#include "Player/GP_PlayerController.h"
#include "TimerManager.h"

AGP_ShrineRuinsRegionEventActor::AGP_ShrineRuinsRegionEventActor()
{
	// The shrine should already be visible/active when spawned; overlap claims the reward.
	ActivationRadius = 850.0f;
}

void AGP_ShrineRuinsRegionEventActor::ConfigureGuidedPartyReward(bool bEnabled)
{
	if (!HasAuthority())
	{
		return;
	}

	bGuidedPartyReward = bEnabled;
	if (bGuidedPartyReward)
	{
		// Guided completion is owned by the full dispatch pass instead of the first claimant.
		bCompleteAfterFirstClaim = false;
	}
}

void AGP_ShrineRuinsRegionEventActor::ActivateRegionEvent()
{
	Super::ActivateRegionEvent();

	if (HasAuthority() && GetRuntimeState() == EGPRegionEventRuntimeState::Active)
	{
		if (UWorld* World = GetWorld())
		{
			// Overlap-triggered test stations spawn the shrine around the player, so refresh once physics overlaps settle.
			World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(
				this,
				bGuidedPartyReward
					? &ThisClass::RewardConnectedPartyPlayers
					: &ThisClass::RewardOverlappingPlayers));
		}
	}
}

void AGP_ShrineRuinsRegionEventActor::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && ActivationTrigger)
	{
		ActivationTrigger->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandleShrineOverlap);
	}
}

void AGP_ShrineRuinsRegionEventActor::HandleShrineOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || GetRuntimeState() != EGPRegionEventRuntimeState::Active)
	{
		return;
	}

	TryRewardPlayer(Cast<AGP_PlayerCharacter>(OtherActor));
}

void AGP_ShrineRuinsRegionEventActor::RewardOverlappingPlayers()
{
	if (!HasAuthority() || GetRuntimeState() != EGPRegionEventRuntimeState::Active || !ActivationTrigger)
	{
		return;
	}

	TArray<AActor*> OverlappingPlayers;
	ActivationTrigger->GetOverlappingActors(OverlappingPlayers, AGP_PlayerCharacter::StaticClass());
	for (AActor* OverlappingActor : OverlappingPlayers)
	{
		TryRewardPlayer(Cast<AGP_PlayerCharacter>(OverlappingActor));
		if (GetRuntimeState() != EGPRegionEventRuntimeState::Active)
		{
			break;
		}
	}
}

void AGP_ShrineRuinsRegionEventActor::RewardConnectedPartyPlayers()
{
	if (!HasAuthority() || GetRuntimeState() != EGPRegionEventRuntimeState::Active)
	{
		return;
	}

	TArray<AGP_PlayerController*> PartyControllers;
	if (UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			if (AGP_PlayerController* PlayerController = Cast<AGP_PlayerController>(Iterator->Get()))
			{
				PartyControllers.Add(PlayerController);
			}
		}
	}
	RewardPartyControllers(PartyControllers);
}

void AGP_ShrineRuinsRegionEventActor::RewardPartyControllers(
	const TArray<AGP_PlayerController*>& PartyControllers)
{
	TSet<AGP_PlayerController*> UniqueControllers;
	for (AGP_PlayerController* PlayerController : PartyControllers)
	{
		if (IsValid(PlayerController))
		{
			UniqueControllers.Add(PlayerController);
			TryRewardController(PlayerController);
		}
	}

	if (bGuidedPartyReward
		&& UniqueControllers.Num() > 0
		&& RewardedControllers.Num() >= UniqueControllers.Num())
	{
		// RPC dispatch is synchronous on the server; the flow adds presentation grace before the boss beat.
		CompleteRegionEvent();
	}
}

void AGP_ShrineRuinsRegionEventActor::TryRewardPlayer(AGP_PlayerCharacter* PlayerCharacter)
{
	if (!IsValid(PlayerCharacter))
	{
		return;
	}

	if (TryRewardController(Cast<AGP_PlayerController>(PlayerCharacter->GetController()))
		&& bCompleteAfterFirstClaim)
	{
		CompleteRegionEvent();
	}
}

bool AGP_ShrineRuinsRegionEventActor::TryRewardController(AGP_PlayerController* PlayerController)
{
	if (!IsValid(PlayerController) || RewardedControllers.Contains(PlayerController))
	{
		return false;
	}

	RewardedControllers.Add(PlayerController);
	// Each controller opens the actual widget locally so every multiplayer client receives independent choices.
	PlayerController->ClientOpenRegionEventAugmentSelect();
	return true;
}
