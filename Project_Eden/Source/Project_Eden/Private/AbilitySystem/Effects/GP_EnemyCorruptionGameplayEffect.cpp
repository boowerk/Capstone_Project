#include "AbilitySystem/Effects/GP_EnemyCorruptionGameplayEffect.h"

#include "AbilitySystem/GP_AttributeSet.h"
#include "GameplayTags/GP_Tags.h"

UGP_EnemyCorruptionGameplayEffect::UGP_EnemyCorruptionGameplayEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	FSetByCallerFloat DamageIncreaseMagnitude;
	DamageIncreaseMagnitude.DataTag = GPTags::Corruption::Data::DamageIncreaseRate;

	FGameplayModifierInfo DamageIncreaseModifier;
	DamageIncreaseModifier.Attribute = UGP_AttributeSet::GetDamageIncreaseRateAttribute();
	DamageIncreaseModifier.ModifierOp = EGameplayModOp::Additive;
	DamageIncreaseModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(DamageIncreaseMagnitude);
	Modifiers.Add(DamageIncreaseModifier);

	FSetByCallerFloat ArmorBonusMagnitude;
	ArmorBonusMagnitude.DataTag = GPTags::Corruption::Data::ArmorBonus;

	FGameplayModifierInfo ArmorBonusModifier;
	ArmorBonusModifier.Attribute = UGP_AttributeSet::GetArmorAttribute();
	ArmorBonusModifier.ModifierOp = EGameplayModOp::Additive;
	ArmorBonusModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(ArmorBonusMagnitude);
	Modifiers.Add(ArmorBonusModifier);
}
