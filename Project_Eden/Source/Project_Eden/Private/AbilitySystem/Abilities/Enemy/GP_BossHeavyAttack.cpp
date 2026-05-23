#include "AbilitySystem/Abilities/Enemy/GP_BossHeavyAttack.h"

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

	// Sans does not have a dedicated heavy montage yet, so this branch falls back to the boss AnimationSet scratch montage.

	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageEffectFinder(TEXT("/Game/GAS_Pattern/AbilitySystem/GameplayEffects/GE_PrimaryDamage"));
	if (DamageEffectFinder.Succeeded())
	{
		DamageEffectClass = DamageEffectFinder.Class;
	}
}
