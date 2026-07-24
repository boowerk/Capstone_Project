#include "AbilitySystem/Abilities/Enemy/GP_BossHeavyAttack.h"

#include "Animation/AnimMontage.h"
#include "Animation/PDA_CharacterAnimationSet.h"
#include "Characters/GP_BaseCharacter.h"
#include "GameplayEffect.h"
#include "GameplayTags/GP_Tags.h"
#include "UObject/ConstructorHelpers.h"

UGP_BossHeavyAttack::UGP_BossHeavyAttack()
{
	// Heavy attack stays as a tighter frontal strike for the close-range punish branch.
	FGameplayTagContainer AbilityAssetTags;
	AbilityAssetTags.AddTag(GPTags::Ability::Enemy::Attack_BossHeavy);
	SetAssetTags(AbilityAssetTags);

	AttackRadius = 320.0f;
	ForwardOffset = 220.0f;
	HitBoxElevationOffset = 40.0f;
	bUseGameplayEventForHitTiming = false;

	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageEffectFinder(TEXT("/Game/GAS_Pattern/AbilitySystem/GameplayEffects/Damage/GE_PrimaryDamage"));
	if (DamageEffectFinder.Succeeded())
	{
		DamageEffectClass = DamageEffectFinder.Class;
	}
}

UAnimMontage* UGP_BossHeavyAttack::ResolveConfiguredAttackMontage(AActor* AvatarActor) const
{
	const AGP_BaseCharacter* BaseCharacter = Cast<AGP_BaseCharacter>(AvatarActor);
	const UPDA_CharacterAnimationSet* AnimationSet = IsValid(BaseCharacter) ? BaseCharacter->AnimationSet : nullptr;
	return IsValid(AnimationSet) && IsValid(AnimationSet->BossHeavyAttackMontage)
		? AnimationSet->BossHeavyAttackMontage.Get()
		: Super::ResolveConfiguredAttackMontage(AvatarActor);
}
