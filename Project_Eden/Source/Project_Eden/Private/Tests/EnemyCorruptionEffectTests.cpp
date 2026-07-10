#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/Effects/GP_EnemyCorruptionGameplayEffect.h"

#include "AbilitySystem/GP_AttributeSet.h"
#include "GameplayTags/GP_Tags.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPEnemyCorruptionEffectContractTest,
	"ProjectEden.Game.Corruption.EnemyEffectContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPEnemyCorruptionEffectContractTest::RunTest(const FString& Parameters)
{
	const UGP_EnemyCorruptionGameplayEffect* Effect = GetDefault<UGP_EnemyCorruptionGameplayEffect>();
	TestNotNull(TEXT("Native corruption effect CDO exists"), Effect);
	if (!Effect)
	{
		return false;
	}

	TestEqual(TEXT("Corruption buff persists until its owning adapter replaces it"), Effect->DurationPolicy, EGameplayEffectDurationType::Infinite);
	TestEqual(TEXT("Corruption buff owns exactly damage and armor contributions"), Effect->Modifiers.Num(), 2);
	if (Effect->Modifiers.Num() != 2)
	{
		return false;
	}

	const FGameplayModifierInfo& DamageModifier = Effect->Modifiers[0];
	TestEqual(TEXT("First modifier targets the shared outgoing damage-rate attribute"), DamageModifier.Attribute, UGP_AttributeSet::GetDamageIncreaseRateAttribute());
	TestEqual(TEXT("Damage modifier is additive with other buffs"), DamageModifier.ModifierOp, EGameplayModOp::Additive);
	TestTrue(
		TEXT("Damage magnitude comes from the corruption SetByCaller tag"),
		DamageModifier.ModifierMagnitude.GetSetByCallerFloat().DataTag == GPTags::Corruption::Data::DamageIncreaseRate);

	const FGameplayModifierInfo& ArmorModifier = Effect->Modifiers[1];
	TestEqual(TEXT("Second modifier targets armor"), ArmorModifier.Attribute, UGP_AttributeSet::GetArmorAttribute());
	TestEqual(TEXT("Armor modifier is additive so zero-base enemies are strengthened"), ArmorModifier.ModifierOp, EGameplayModOp::Additive);
	TestTrue(
		TEXT("Armor magnitude comes from the corruption SetByCaller tag"),
		ArmorModifier.ModifierMagnitude.GetSetByCallerFloat().DataTag == GPTags::Corruption::Data::ArmorBonus);

	return true;
}

#endif
