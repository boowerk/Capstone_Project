#include "AbilitySystem/Abilities/Enemy/GP_MatadorBullPatternAbility.h"

#include "AI/Debug/EnemyAIDebugUtils.h"
#include "Characters/GP_MatadorMageBossCharacter.h"
#include "GameplayTags/GP_Tags.h"

UGP_MatadorBullPatternAbility::UGP_MatadorBullPatternAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// The boss selector activates this through a Matador-specific utility tag.
	FGameplayTagContainer AbilityAssetTags;
	AbilityAssetTags.AddTag(GPTags::Ability::Enemy::Utility_MatadorBullPattern);
	SetAssetTags(AbilityAssetTags);
}

void UGP_MatadorBullPatternAbility::ActivateAbility(
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

	AGP_MatadorMageBossCharacter* MatadorBoss = Cast<AGP_MatadorMageBossCharacter>(GetAvatarActorFromActorInfo());
	const bool bStarted = HasAuthority(&ActivationInfo) && IsValid(MatadorBoss) && MatadorBoss->RequestStartBullPattern(nullptr);
	UE_LOG(
		LogEnemyAI,
		Log,
		TEXT("[BossAI] Matador bull pattern ability requested. Boss=%s Started=%d"),
		*GetNameSafe(MatadorBoss),
		bStarted ? 1 : 0);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, !bStarted);
}
