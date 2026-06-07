#include "AbilitySystem/Abilities/Enemy/GP_MatadorGroggyAbility.h"

#include "AI/Debug/EnemyAIDebugUtils.h"
#include "Characters/GP_MatadorMageBossCharacter.h"
#include "GameplayTags/GP_Tags.h"

UGP_MatadorGroggyAbility::UGP_MatadorGroggyAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// Groggy is represented as a GAS ability so BT/debug tooling sees it as a selected boss result.
	FGameplayTagContainer AbilityAssetTags;
	AbilityAssetTags.AddTag(GPTags::Ability::Enemy::Utility_MatadorGroggy);
	SetAssetTags(AbilityAssetTags);
}

void UGP_MatadorGroggyAbility::ActivateAbility(
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
	const bool bEnteredGroggy = HasAuthority(&ActivationInfo) && IsValid(MatadorBoss);
	if (bEnteredGroggy)
	{
		MatadorBoss->RequestEnterGroggy();
	}

	UE_LOG(
		LogEnemyAI,
		Log,
		TEXT("[BossAI] Matador groggy ability requested. Boss=%s Entered=%d"),
		*GetNameSafe(MatadorBoss),
		bEnteredGroggy ? 1 : 0);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, !bEnteredGroggy);
}
