#include "AbilitySystem/Abilities/Enemy/GP_BossBasicAttack.h"

#include "GameplayEffect.h"
#include "GameplayTags/GP_Tags.h"
#include "UObject/ConstructorHelpers.h"

UGP_BossBasicAttack::UGP_BossBasicAttack()
{
	// Boss basic keeps the shared melee tag so the common BT attack entry can still activate it.
	FGameplayTagContainer AbilityAssetTags;
	AbilityAssetTags.AddTag(GPTags::Ability::Enemy::Attack_Melee);
	SetAssetTags(AbilityAssetTags);

	// Sans basic attack uses the AnimationSet primary montage, which is generated as AM_Sans_Zombie_Scratch.
	AttackRadius = 180.0f;
	ForwardOffset = 160.0f;
	HitBoxElevationOffset = 40.0f;
	bUseGameplayEventForHitTiming = false;

	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageEffectFinder(TEXT("/Game/GAS_Pattern/AbilitySystem/GameplayEffects/Damage/GE_PrimaryDamage"));
	if (DamageEffectFinder.Succeeded())
	{
		DamageEffectClass = DamageEffectFinder.Class;
	}
}
