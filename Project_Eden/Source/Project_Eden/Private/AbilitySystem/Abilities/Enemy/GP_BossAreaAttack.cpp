#include "AbilitySystem/Abilities/Enemy/GP_BossAreaAttack.h"

#include "Animation/AnimMontage.h"
#include "Animation/PDA_CharacterAnimationSet.h"
#include "Characters/GP_BaseCharacter.h"
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

	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageEffectFinder(TEXT("/Game/GAS_Pattern/AbilitySystem/GameplayEffects/Damage/GE_PrimaryDamage"));
	if (DamageEffectFinder.Succeeded())
	{
		DamageEffectClass = DamageEffectFinder.Class;
	}
}

UAnimMontage* UGP_BossAreaAttack::ResolveConfiguredAttackMontage(AActor* AvatarActor) const
{
	const AGP_BaseCharacter* BaseCharacter = Cast<AGP_BaseCharacter>(AvatarActor);
	const UPDA_CharacterAnimationSet* AnimationSet = IsValid(BaseCharacter) ? BaseCharacter->AnimationSet : nullptr;
	return IsValid(AnimationSet) && IsValid(AnimationSet->BossAreaAttackMontage)
		? AnimationSet->BossAreaAttackMontage.Get()
		: Super::ResolveConfiguredAttackMontage(AvatarActor);
}
