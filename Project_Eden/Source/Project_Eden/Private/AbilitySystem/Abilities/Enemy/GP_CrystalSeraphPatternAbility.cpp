#include "AbilitySystem/Abilities/Enemy/GP_CrystalSeraphPatternAbility.h"

#include "AI/Debug/EnemyAIDebugUtils.h"
#include "Characters/GP_CrystalSeraphBossCharacter.h"
#include "GameplayTags/GP_Tags.h"

UGP_CrystalSeraphPatternAbility::UGP_CrystalSeraphPatternAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
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
	const bool bStarted = HasAuthority(&ActivationInfo) && IsValid(CrystalSeraphBoss) && ExecuteCrystalSeraphPattern(CrystalSeraphBoss, TriggerEventData);
	UE_LOG(
		LogEnemyAI,
		Log,
		TEXT("[BossAI] Crystal Seraph pattern requested. Ability=%s Boss=%s Started=%d"),
		*GetNameSafe(this),
		*EnemyAIDebugUtils::DescribeActor(CrystalSeraphBoss),
		bStarted ? 1 : 0);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, !bStarted);
}

bool UGP_CrystalSeraphPatternAbility::ExecuteCrystalSeraphPattern(AGP_CrystalSeraphBossCharacter* CrystalSeraphBoss, const FGameplayEventData* TriggerEventData)
{
	return false;
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

bool UGP_CrystalSeraphShardAbility::ExecuteCrystalSeraphPattern(AGP_CrystalSeraphBossCharacter* CrystalSeraphBoss, const FGameplayEventData* TriggerEventData)
{
	return IsValid(CrystalSeraphBoss) && CrystalSeraphBoss->RequestSpawnCrystalShardPattern(ResolvePatternTarget(TriggerEventData));
}

UGP_CrystalSeraphLaserAbility::UGP_CrystalSeraphLaserAbility()
{
	FGameplayTagContainer AbilityAssetTags;
	AbilityAssetTags.AddTag(GPTags::Ability::Boss::CrystalSeraph::Laser);
	SetAssetTags(AbilityAssetTags);
}

bool UGP_CrystalSeraphLaserAbility::ExecuteCrystalSeraphPattern(AGP_CrystalSeraphBossCharacter* CrystalSeraphBoss, const FGameplayEventData* TriggerEventData)
{
	return IsValid(CrystalSeraphBoss) && CrystalSeraphBoss->RequestStartLaserPattern(ResolvePatternTarget(TriggerEventData));
}

UGP_CrystalSeraphPrismAbility::UGP_CrystalSeraphPrismAbility()
{
	FGameplayTagContainer AbilityAssetTags;
	AbilityAssetTags.AddTag(GPTags::Ability::Boss::CrystalSeraph::Prism);
	SetAssetTags(AbilityAssetTags);
}

bool UGP_CrystalSeraphPrismAbility::ExecuteCrystalSeraphPattern(AGP_CrystalSeraphBossCharacter* CrystalSeraphBoss, const FGameplayEventData* TriggerEventData)
{
	return IsValid(CrystalSeraphBoss) && IsValid(CrystalSeraphBoss->RequestSpawnCrystalPrism(ResolvePatternTarget(TriggerEventData)));
}

UGP_CrystalSeraphAreaAbility::UGP_CrystalSeraphAreaAbility()
{
	FGameplayTagContainer AbilityAssetTags;
	AbilityAssetTags.AddTag(GPTags::Ability::Boss::CrystalSeraph::Area);
	SetAssetTags(AbilityAssetTags);
}

bool UGP_CrystalSeraphAreaAbility::ExecuteCrystalSeraphPattern(AGP_CrystalSeraphBossCharacter* CrystalSeraphBoss, const FGameplayEventData* TriggerEventData)
{
	return IsValid(CrystalSeraphBoss) && CrystalSeraphBoss->RequestStartAreaPattern(ResolvePatternTarget(TriggerEventData));
}

UGP_CrystalSeraphGroggyAbility::UGP_CrystalSeraphGroggyAbility()
{
	FGameplayTagContainer AbilityAssetTags;
	AbilityAssetTags.AddTag(GPTags::Ability::Boss::CrystalSeraph::Groggy);
	SetAssetTags(AbilityAssetTags);
}

bool UGP_CrystalSeraphGroggyAbility::ExecuteCrystalSeraphPattern(AGP_CrystalSeraphBossCharacter* CrystalSeraphBoss, const FGameplayEventData* TriggerEventData)
{
	if (!IsValid(CrystalSeraphBoss))
	{
		return false;
	}

	CrystalSeraphBoss->RequestEnterGroggy();
	return true;
}
