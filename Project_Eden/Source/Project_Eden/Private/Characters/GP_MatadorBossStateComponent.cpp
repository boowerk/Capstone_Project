#include "Characters/GP_MatadorBossStateComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Actors/GP_BullChargeActor.h"
#include "GameplayTags/GP_Tags.h"
#include "Net/UnrealNetwork.h"

UGP_MatadorBossStateComponent::UGP_MatadorBossStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UGP_MatadorBossStateComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!MainBossActor)
	{
		MainBossActor = GetOwner();
	}

	// The initial guarded tag is applied here so spawned Blueprint bosses get the same damage rules as native ones.
	if (GetOwnerRole() == ROLE_Authority)
	{
		ApplyStateTags();
	}
}

void UGP_MatadorBossStateComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Clear loose tags owned by this component before the ASC/owner goes away.
	SetGuardedInternal(false);
	SetGroggyInternal(false);

	Super::EndPlay(EndPlayReason);
}

void UGP_MatadorBossStateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGP_MatadorBossStateComponent, ChainBreakCount);
	DOREPLIFETIME(UGP_MatadorBossStateComponent, bIsGroggy);
	DOREPLIFETIME(UGP_MatadorBossStateComponent, bIsGuarded);
	DOREPLIFETIME(UGP_MatadorBossStateComponent, MainBossActor);
	DOREPLIFETIME(UGP_MatadorBossStateComponent, DecoyActor);
	DOREPLIFETIME(UGP_MatadorBossStateComponent, ChainEffectActor);
	DOREPLIFETIME(UGP_MatadorBossStateComponent, ActiveBullActor);
}

void UGP_MatadorBossStateComponent::InitializeMatadorState(AActor* InMainBossActor)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	MainBossActor = InMainBossActor ? InMainBossActor : GetOwner();
	ResetChainBreakCount();
	SetGroggyInternal(false);
	SetGuardedInternal(true);
}

void UGP_MatadorBossStateComponent::RegisterDecoyActor(AActor* InDecoyActor)
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		DecoyActor = InDecoyActor;
	}
}

void UGP_MatadorBossStateComponent::RegisterChainEffectActor(AActor* InChainEffectActor)
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		ChainEffectActor = InChainEffectActor;
	}
}

void UGP_MatadorBossStateComponent::RegisterActiveBullActor(AActor* InBullActor)
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		if (UAbilitySystemComponent* ASC = ResolveOwnerASC())
		{
			if (InBullActor)
			{
				ASC->AddLooseGameplayTag(GPTags::State::Status::Enemy::MatadorMeleeActive);
			}
			else
			{
				ASC->RemoveLooseGameplayTag(GPTags::State::Status::Enemy::MatadorMeleeActive);
			}
		}

		ActiveBullActor = InBullActor;
	}
}

bool UGP_MatadorBossStateComponent::TryRedirectActiveBullTowardDecoy(AActor* RedirectingActor)
{
	if (GetOwnerRole() != ROLE_Authority || bIsGroggy)
	{
		return false;
	}

	AGP_BullChargeActor* BullActor = Cast<AGP_BullChargeActor>(ActiveBullActor.Get());
	if (!IsValid(BullActor))
	{
		return false;
	}

	return BullActor->TryRedirectTowardDecoy(RedirectingActor);
}

void UGP_MatadorBossStateComponent::RecordBullHitDecoy()
{
	if (GetOwnerRole() != ROLE_Authority || bIsGroggy)
	{
		return;
	}

	SetChainBreakCount(ChainBreakCount + 1);
	if (ChainBreakCount >= GetChainBreakTarget())
	{
		EnterGroggy();
	}
}

void UGP_MatadorBossStateComponent::SetChainBreakCount(int32 NewCount)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	const int32 ClampedCount = FMath::Clamp(NewCount, 0, GetChainBreakTarget());
	if (ChainBreakCount == ClampedCount)
	{
		return;
	}

	ChainBreakCount = ClampedCount;
	OnChainStageChanged.Broadcast(ChainBreakCount, GetChainBreakTarget());
}

void UGP_MatadorBossStateComponent::ResetChainBreakCount()
{
	SetChainBreakCount(0);
}

void UGP_MatadorBossStateComponent::EnterGroggy()
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	SetChainBreakCount(GetChainBreakTarget());
	SetGuardedInternal(false);
	SetGroggyInternal(true);
}

void UGP_MatadorBossStateComponent::RecoverFromGroggy()
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	ResetChainBreakCount();
	SetGroggyInternal(false);
	SetGuardedInternal(true);
}

void UGP_MatadorBossStateComponent::OnRep_ChainBreakCount()
{
	OnChainStageChanged.Broadcast(ChainBreakCount, GetChainBreakTarget());
}

void UGP_MatadorBossStateComponent::OnRep_IsGroggy()
{
	// Groggy also changes whether the guarded tag should be visible on this client.
	ApplyStateTags();
	OnGroggyChanged.Broadcast(bIsGroggy);
}

void UGP_MatadorBossStateComponent::OnRep_IsGuarded()
{
	ApplyStateTags();
}

void UGP_MatadorBossStateComponent::SetGroggyInternal(bool bNewGroggy)
{
	if (bIsGroggy == bNewGroggy)
	{
		return;
	}

	bIsGroggy = bNewGroggy;
	ApplyStateTags();
	OnGroggyChanged.Broadcast(bIsGroggy);
}

void UGP_MatadorBossStateComponent::SetGuardedInternal(bool bNewGuarded)
{
	if (bIsGuarded == bNewGuarded)
	{
		return;
	}

	bIsGuarded = bNewGuarded;
	ApplyStateTags();
}

void UGP_MatadorBossStateComponent::ApplyStateTags()
{
	UAbilitySystemComponent* ASC = ResolveOwnerASC();
	if (!ASC)
	{
		return;
	}

	const bool bShouldApplyGuarded = bIsGuarded && !bIsGroggy;
	if (bShouldApplyGuarded && !bAppliedGuardedTag)
	{
		ASC->AddLooseGameplayTag(GPTags::State::Status::Enemy::MatadorGuarded);
		bAppliedGuardedTag = true;
	}
	else if (!bShouldApplyGuarded && bAppliedGuardedTag)
	{
		ASC->RemoveLooseGameplayTag(GPTags::State::Status::Enemy::MatadorGuarded);
		bAppliedGuardedTag = false;
	}

	if (bIsGroggy && !bAppliedGroggyTag)
	{
		ASC->AddLooseGameplayTag(GPTags::State::Status::Enemy::Groggy);
		bAppliedGroggyTag = true;
	}
	else if (!bIsGroggy && bAppliedGroggyTag)
	{
		ASC->RemoveLooseGameplayTag(GPTags::State::Status::Enemy::Groggy);
		bAppliedGroggyTag = false;
	}
}

UAbilitySystemComponent* UGP_MatadorBossStateComponent::ResolveOwnerASC() const
{
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
}
