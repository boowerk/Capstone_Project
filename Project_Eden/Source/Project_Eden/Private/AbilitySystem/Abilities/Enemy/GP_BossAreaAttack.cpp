#include "AbilitySystem/Abilities/Enemy/GP_BossAreaAttack.h"

#include "GameplayEffect.h"
#include "GameplayTags/GP_Tags.h"
#include "UObject/ConstructorHelpers.h"

UGP_BossAreaAttack::UGP_BossAreaAttack()
{
	// Boss area uses a centered circle so Sans can punish players who stay in mid range during later phases.
	FGameplayTagContainer AbilityAssetTags;
	AbilityAssetTags.AddTag(GPTags::Ability::Enemy::Attack_BossArea);
	SetAssetTags(AbilityAssetTags);

	AttackRadius = 850.0f;
	ForwardOffset = 0.0f;
	HitBoxElevationOffset = 35.0f;
	bUseGameplayEventForHitTiming = false;

	// Area currently uses the data-asset fallback animation instead of the old placeholder montage path.

	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageEffectFinder(TEXT("/Game/GAS_Pattern/AbilitySystem/GameplayEffects/Damage/GE_PrimaryDamage"));
	if (DamageEffectFinder.Succeeded())
	{
		DamageEffectClass = DamageEffectFinder.Class;
	}
}
