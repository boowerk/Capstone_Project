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

void AGP_ShrineRuinsRegionEventActor::ActivateRegionEvent()
{
	Super::ActivateRegionEvent();

	if (HasAuthority() && GetRuntimeState() == EGPRegionEventRuntimeState::Active)
	{
		if (UWorld* World = GetWorld())
		{
			// Overlap-triggered test stations spawn the shrine around the player, so refresh once physics overlaps settle.
			World->GetTimerManager().SetTimerForNextTick(
				FTimerDelegate::CreateUObject(this, &ThisClass::RewardOverlappingPlayers));
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

void AGP_ShrineRuinsRegionEventActor::TryRewardPlayer(AGP_PlayerCharacter* PlayerCharacter)
{
	if (!IsValid(PlayerCharacter) || RewardedPlayers.Contains(PlayerCharacter))
	{
		return;
	}

	RewardedPlayers.Add(PlayerCharacter);
	if (AGP_PlayerController* PlayerController = Cast<AGP_PlayerController>(PlayerCharacter->GetController()))
	{
		// The controller opens the actual widget locally so multiplayer clients receive their own choices.
		PlayerController->ClientOpenRegionEventAugmentSelect();
	}

	if (bCompleteAfterFirstClaim)
	{
		CompleteRegionEvent();
	}
}
