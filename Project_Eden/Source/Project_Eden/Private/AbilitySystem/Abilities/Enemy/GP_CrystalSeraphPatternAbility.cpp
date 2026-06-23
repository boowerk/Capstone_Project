#include "AbilitySystem/Abilities/Enemy/GP_CrystalSeraphPatternAbility.h"

#include "AI/Debug/EnemyAIDebugUtils.h"
#include "Characters/GP_CrystalSeraphBossCharacter.h"
#include "GameplayTags/GP_Tags.h"
#include "VFX/GP_BossTelegraphVFXComponent.h"

UGP_CrystalSeraphPatternAbility::UGP_CrystalSeraphPatternAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UGP_CrystalSeraphPatternAbility::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const AGP_CrystalSeraphBossCharacter* CrystalSeraphBoss = ActorInfo != nullptr
		? Cast<AGP_CrystalSeraphBossCharacter>(ActorInfo->AvatarActor.Get())
		: nullptr;
	// Reject stale BT attack requests at GAS activation time, not only when the tactics service refreshes Blackboard.
	return !UsesSharedPatternCadence()
		|| (IsValid(CrystalSeraphBoss) && CrystalSeraphBoss->CanStartCrystalSeraphPattern());
}

void UGP_CrystalSeraphPatternAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AGP_CrystalSeraphBossCharacter* CrystalSeraphBoss = Cast<AGP_CrystalSeraphBossCharacter>(GetAvatarActorFromActorInfo());
	const bool bCadenceReserved = !UsesSharedPatternCadence()
		|| (HasAuthority(&ActivationInfo) && IsValid(CrystalSeraphBoss) && CrystalSeraphBoss->TryStartCrystalSeraphPattern());
	if (!bCadenceReserved || !HasAuthority(&ActivationInfo) || !IsValid(CrystalSeraphBoss))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	PendingPatternTarget = ResolvePatternTarget(TriggerEventData);
	const float TelegraphDelay = UsesBossTelegraphVFX()
		? CrystalSeraphBoss->PlayBossTelegraphForPattern(GetPatternTag())
		: 0.0f;
	if (TelegraphDelay > KINDA_SMALL_NUMBER)
	{
		// Keep the GAS ability alive until the visual lead-in completes, then execute the original pattern boundary.
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				TelegraphTimerHandle,
				this,
				&ThisClass::ExecutePendingPattern,
				TelegraphDelay,
				false);
			return;
		}
	}

	ExecutePendingPattern();
}

void UGP_CrystalSeraphPatternAbility::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TelegraphTimerHandle);
	}
	PendingPatternTarget.Reset();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGP_CrystalSeraphPatternAbility::ExecutePendingPattern()
{
	AGP_CrystalSeraphBossCharacter* CrystalSeraphBoss = Cast<AGP_CrystalSeraphBossCharacter>(GetAvatarActorFromActorInfo());
	const bool bStarted = HasAuthority(&CurrentActivationInfo)
		&& IsValid(CrystalSeraphBoss)
		&& ExecuteCrystalSeraphPattern(CrystalSeraphBoss, PendingPatternTarget.Get());
	UE_LOG(
		LogEnemyAI,
		Log,
		TEXT("[BossAI] Crystal Seraph pattern requested. Ability=%s Boss=%s Started=%d"),
		*GetNameSafe(this),
		*EnemyAIDebugUtils::DescribeActor(CrystalSeraphBoss),
		bStarted ? 1 : 0);

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, !bStarted);
}

bool UGP_CrystalSeraphPatternAbility::ExecuteCrystalSeraphPattern(AGP_CrystalSeraphBossCharacter* CrystalSeraphBoss, AActor* PatternTargetActor)
{
	return false;
}

FGameplayTag UGP_CrystalSeraphPatternAbility::GetPatternTag() const
{
	// Each concrete Crystal Seraph GAS ability owns exactly one pattern asset tag.
	for (const FGameplayTag& Tag : GetAssetTags())
	{
		return Tag;
	}
	return FGameplayTag();
}

AActor* UGP_CrystalSeraphPatternAbility::ResolvePatternTarget(const FGameplayEventData* TriggerEventData) const
{
	return TriggerEventData != nullptr ? const_cast<AActor*>(TriggerEventData->Target.Get()) : nullptr;
}

UGP_CrystalSeraphShardAbility::UGP_CrystalSeraphShardAbility()
{
	FGameplayTagContainer AbilityAssetTags;
	AbilityAssetTags.AddTag(GPTags::Ability::Boss::CrystalSeraph::Basic);
	SetAssetTags(AbilityAssetTags);
}

bool UGP_CrystalSeraphShardAbility::ExecuteCrystalSeraphPattern(AGP_CrystalSeraphBossCharacter* CrystalSeraphBoss, AActor* PatternTargetActor)
{
	return IsValid(CrystalSeraphBoss) && CrystalSeraphBoss->RequestSpawnCrystalShardPattern(PatternTargetActor);
}

UGP_CrystalSeraphLaserAbility::UGP_CrystalSeraphLaserAbility()
{
	FGameplayTagContainer AbilityAssetTags;
	AbilityAssetTags.AddTag(GPTags::Ability::Boss::CrystalSeraph::Laser);
	SetAssetTags(AbilityAssetTags);
}

bool UGP_CrystalSeraphLaserAbility::ExecuteCrystalSeraphPattern(AGP_CrystalSeraphBossCharacter* CrystalSeraphBoss, AActor* PatternTargetActor)
{
	return IsValid(CrystalSeraphBoss) && CrystalSeraphBoss->RequestStartLaserPattern(PatternTargetActor);
}

UGP_CrystalSeraphPrismAbility::UGP_CrystalSeraphPrismAbility()
{
	FGameplayTagContainer AbilityAssetTags;
	AbilityAssetTags.AddTag(GPTags::Ability::Boss::CrystalSeraph::Prism);
	SetAssetTags(AbilityAssetTags);
}

bool UGP_CrystalSeraphPrismAbility::ExecuteCrystalSeraphPattern(AGP_CrystalSeraphBossCharacter* CrystalSeraphBoss, AActor* PatternTargetActor)
{
	return IsValid(CrystalSeraphBoss) && IsValid(CrystalSeraphBoss->RequestSpawnCrystalPrism(PatternTargetActor));
}

UGP_CrystalSeraphAreaAbility::UGP_CrystalSeraphAreaAbility()
{
	FGameplayTagContainer AbilityAssetTags;
	AbilityAssetTags.AddTag(GPTags::Ability::Boss::CrystalSeraph::Area);
	SetAssetTags(AbilityAssetTags);
}

bool UGP_CrystalSeraphAreaAbility::ExecuteCrystalSeraphPattern(AGP_CrystalSeraphBossCharacter* CrystalSeraphBoss, AActor* PatternTargetActor)
{
	return IsValid(CrystalSeraphBoss) && CrystalSeraphBoss->RequestStartAreaPattern(PatternTargetActor);
}

UGP_CrystalSeraphGroggyAbility::UGP_CrystalSeraphGroggyAbility()
{
	FGameplayTagContainer AbilityAssetTags;
	AbilityAssetTags.AddTag(GPTags::Ability::Boss::CrystalSeraph::Groggy);
	SetAssetTags(AbilityAssetTags);
}

bool UGP_CrystalSeraphGroggyAbility::ExecuteCrystalSeraphPattern(AGP_CrystalSeraphBossCharacter* CrystalSeraphBoss, AActor* PatternTargetActor)
{
	if (!IsValid(CrystalSeraphBoss))
	{
		return false;
	}

	CrystalSeraphBoss->RequestEnterGroggy();
	return true;
}

UGP_CrystalSeraphTeleportAbility::UGP_CrystalSeraphTeleportAbility()
{
	FGameplayTagContainer AbilityAssetTags;
	AbilityAssetTags.AddTag(GPTags::Ability::Boss::CrystalSeraph::Teleport);
	SetAssetTags(AbilityAssetTags);
}

bool UGP_CrystalSeraphTeleportAbility::ExecuteCrystalSeraphPattern(AGP_CrystalSeraphBossCharacter* CrystalSeraphBoss, AActor* PatternTargetActor)
{
	return IsValid(CrystalSeraphBoss) && CrystalSeraphBoss->RequestTeleportToPreferredCombatPosition(PatternTargetActor);
}
