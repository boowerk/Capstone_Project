#include "AbilitySystem/GP_DamageExecCalculation.h"
#include "AbilitySystem/GP_AttributeSet.h"
#include "GameplayTags/GP_Tags.h"
#include "AbilitySystemComponent.h"
#include "HAL/IConsoleManager.h"
#include "Math/UnrealMathUtility.h"
#include "Characters/GP_DarkArmorKnightStateComponent.h"

static TAutoConsoleVariable<int32> CVarGPDamageExecLog(
    TEXT("gp.DamageExec.Log"),
    0,
    TEXT("Logs GP damage execution values when non-zero."),
    ECVF_Default);

namespace GPDamageExec
{
    constexpr float MatadorGuardedFinalDamageMultiplier = 0.1f;
    constexpr float CrystalGuardedFinalDamageMultiplier = 0.15f;
    constexpr float CrystalWingCoreExposedFinalDamageMultiplier = 0.5f;
}

struct FGP_DamageStatics
{
    DECLARE_ATTRIBUTE_CAPTUREDEF(AttackPower);
    DECLARE_ATTRIBUTE_CAPTUREDEF(MagicPower);
    DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalChance);
    DECLARE_ATTRIBUTE_CAPTUREDEF(CritMultiplier);
    DECLARE_ATTRIBUTE_CAPTUREDEF(DamageIncreaseRate);

    DECLARE_ATTRIBUTE_CAPTUREDEF(Armor);
    DECLARE_ATTRIBUTE_CAPTUREDEF(MaxHealth);
    
    DECLARE_ATTRIBUTE_CAPTUREDEF(PyrosResistance);
    DECLARE_ATTRIBUTE_CAPTUREDEF(HydroResistance);
    DECLARE_ATTRIBUTE_CAPTUREDEF(VoltResistance);
    DECLARE_ATTRIBUTE_CAPTUREDEF(AeroResistance);
    DECLARE_ATTRIBUTE_CAPTUREDEF(LuxResistance);
    DECLARE_ATTRIBUTE_CAPTUREDEF(ChaosResistance);
    DECLARE_ATTRIBUTE_CAPTUREDEF(BruteResistance);

    FGP_DamageStatics()
    {
        // 소스 캡처
        DEFINE_ATTRIBUTE_CAPTUREDEF(UGP_AttributeSet, AttackPower, Source, false);
        DEFINE_ATTRIBUTE_CAPTUREDEF(UGP_AttributeSet, MagicPower, Source, false);
        DEFINE_ATTRIBUTE_CAPTUREDEF(UGP_AttributeSet, CriticalChance, Source, false);
        DEFINE_ATTRIBUTE_CAPTUREDEF(UGP_AttributeSet, CritMultiplier, Source, false);
        DEFINE_ATTRIBUTE_CAPTUREDEF(UGP_AttributeSet, DamageIncreaseRate, Source, false);

        // 타겟 캡처
        DEFINE_ATTRIBUTE_CAPTUREDEF(UGP_AttributeSet, Armor, Target, false);
        DEFINE_ATTRIBUTE_CAPTUREDEF(UGP_AttributeSet, MaxHealth, Target, false);
        
        DEFINE_ATTRIBUTE_CAPTUREDEF(UGP_AttributeSet, PyrosResistance, Target, false);
        DEFINE_ATTRIBUTE_CAPTUREDEF(UGP_AttributeSet, HydroResistance, Target, false);
        DEFINE_ATTRIBUTE_CAPTUREDEF(UGP_AttributeSet, VoltResistance, Target, false);
        DEFINE_ATTRIBUTE_CAPTUREDEF(UGP_AttributeSet, AeroResistance, Target, false);
        DEFINE_ATTRIBUTE_CAPTUREDEF(UGP_AttributeSet, LuxResistance, Target, false);
        DEFINE_ATTRIBUTE_CAPTUREDEF(UGP_AttributeSet, ChaosResistance, Target, false);
        DEFINE_ATTRIBUTE_CAPTUREDEF(UGP_AttributeSet, BruteResistance, Target, false);
    }
};

static const FGP_DamageStatics& DamageStatics()
{
    static FGP_DamageStatics Statics;
    return Statics;
}

UGP_DamageExecCalculation::UGP_DamageExecCalculation()
{
    RelevantAttributesToCapture.Add(DamageStatics().AttackPowerDef);
    RelevantAttributesToCapture.Add(DamageStatics().MagicPowerDef);
    RelevantAttributesToCapture.Add(DamageStatics().CriticalChanceDef);
    RelevantAttributesToCapture.Add(DamageStatics().CritMultiplierDef);
    RelevantAttributesToCapture.Add(DamageStatics().DamageIncreaseRateDef);

    RelevantAttributesToCapture.Add(DamageStatics().ArmorDef);
    RelevantAttributesToCapture.Add(DamageStatics().MaxHealthDef);

    RelevantAttributesToCapture.Add(DamageStatics().PyrosResistanceDef);
    RelevantAttributesToCapture.Add(DamageStatics().HydroResistanceDef);
    RelevantAttributesToCapture.Add(DamageStatics().VoltResistanceDef);
    RelevantAttributesToCapture.Add(DamageStatics().AeroResistanceDef);
    RelevantAttributesToCapture.Add(DamageStatics().LuxResistanceDef);
    RelevantAttributesToCapture.Add(DamageStatics().ChaosResistanceDef);
    RelevantAttributesToCapture.Add(DamageStatics().BruteResistanceDef);
}

void UGP_DamageExecCalculation::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, OUT FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
    const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
    const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
    const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

    FAggregatorEvaluateParameters EvaluationParameters;
    EvaluationParameters.SourceTags = SourceTags;
    EvaluationParameters.TargetTags = TargetTags;

    // 1. 값 캡처
    float AttackPower = 0.f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().AttackPowerDef, EvaluationParameters, AttackPower);

    float MagicPower = 0.f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().MagicPowerDef, EvaluationParameters, MagicPower);

    float CriticalChance = 0.f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CriticalChanceDef, EvaluationParameters, CriticalChance);

    float CritMultiplier = 0.f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CritMultiplierDef, EvaluationParameters, CritMultiplier);

    float DamageIncreaseRate = 0.f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().DamageIncreaseRateDef, EvaluationParameters, DamageIncreaseRate);

    float Armor = 0.f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().ArmorDef, EvaluationParameters, Armor);

    float MaxHealth = 0.f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().MaxHealthDef, EvaluationParameters, MaxHealth);

    // 2. 가. 기본 피해량 (Damage_Base) 산출
    float BaseDamage = 0.f;
    
    // SetByCaller에서 스킬 계수 가져오기
    float Base = Spec.GetSetByCallerMagnitude(GPTags::Damage::Data::Base, false, 0.0f);
    float C_atk = Spec.GetSetByCallerMagnitude(GPTags::Damage::Coef::Atk, false, 1.0f);
    float C_def = Spec.GetSetByCallerMagnitude(GPTags::Damage::Coef::Def, false, 0.0f);
    float C_hp  = Spec.GetSetByCallerMagnitude(GPTags::Damage::Coef::Hp, false, 0.0f);
    
    // [기존/신규 로직 통합] 물리/마법 분기
    if (Spec.GetDynamicAssetTags().HasTagExact(GPTags::Damage::Type::Magical))
    {
        float M_atk = Spec.GetSetByCallerMagnitude(GPTags::Damage::Coef::M_Atk, false, 1.0f);
        float BaseSpell = Spec.GetSetByCallerMagnitude(GPTags::Damage::Data::BaseSpell, false, 0.0f);
        // 마법력(MagicPower) 우선 참조
        BaseDamage = (MagicPower * M_atk) + (AttackPower * 0.2f) + BaseSpell; 
    }
    else
    {
        // 물리 공식 적용
        BaseDamage = Base + (AttackPower * C_atk) + (Armor * C_def) + (MaxHealth * C_hp);
    }

    // 3. 나. 공격 수정치 적용 (치명타 및 피해 증가)
    const float SkillDamageMultiplier = FMath::Max(
        Spec.GetSetByCallerMagnitude(GPTags::Damage::Data::Multiplier, false, 1.0f),
        0.0f);
    BaseDamage *= SkillDamageMultiplier;

    bool bCritical = FMath::RandRange(0.0f, 1.0f) <= CriticalChance;
    // CritMultiplier 기본값은 1.5로 설정 (기획안)
    float FinalCritMult = bCritical ? FMath::Max(CritMultiplier, 1.5f) : 1.0f;
    
    float Damage_Modified = BaseDamage * FinalCritMult * (1.0f + DamageIncreaseRate);

    // 4. 다/라. 속성 및 방어력 경감
    float Resistance_Elem = 0.0f;
    const TCHAR* ElementName = TEXT("None");
    if (Spec.GetDynamicAssetTags().HasTagExact(GPTags::Damage::Element::Pyros)) { ElementName = TEXT("Pyros"); ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().PyrosResistanceDef, EvaluationParameters, Resistance_Elem); }
    else if (Spec.GetDynamicAssetTags().HasTagExact(GPTags::Damage::Element::Hydro)) { ElementName = TEXT("Hydro"); ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().HydroResistanceDef, EvaluationParameters, Resistance_Elem); }
    else if (Spec.GetDynamicAssetTags().HasTagExact(GPTags::Damage::Element::Volt)) { ElementName = TEXT("Volt"); ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().VoltResistanceDef, EvaluationParameters, Resistance_Elem); }
    else if (Spec.GetDynamicAssetTags().HasTagExact(GPTags::Damage::Element::Aero)) { ElementName = TEXT("Aero"); ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().AeroResistanceDef, EvaluationParameters, Resistance_Elem); }
    else if (Spec.GetDynamicAssetTags().HasTagExact(GPTags::Damage::Element::Lux)) { ElementName = TEXT("Lux"); ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().LuxResistanceDef, EvaluationParameters, Resistance_Elem); }
    else if (Spec.GetDynamicAssetTags().HasTagExact(GPTags::Damage::Element::Chaos)) { ElementName = TEXT("Chaos"); ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().ChaosResistanceDef, EvaluationParameters, Resistance_Elem); }
    else if (Spec.GetDynamicAssetTags().HasTagExact(GPTags::Damage::Element::Brute)) { ElementName = TEXT("Brute"); ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().BruteResistanceDef, EvaluationParameters, Resistance_Elem); }

    float K = 100.0f; 
    float ArmorMitigation = K / (K + FMath::Max(Armor, 0.0f));
    
    float Damage_Final = Damage_Modified * ArmorMitigation * (1.0f - Resistance_Elem);

    // Boss guarded reductions are final-state modifiers: normal combat is reduced, groggy takes full damage.
    const UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
    const bool bCapturedTargetGroggy = TargetTags != nullptr && TargetTags->HasTagExact(GPTags::State::Status::Enemy::Groggy);
    const bool bCapturedTargetMatadorGuarded = TargetTags != nullptr && TargetTags->HasTagExact(GPTags::State::Status::Enemy::MatadorGuarded);
    const bool bCapturedTargetCrystalGuarded = TargetTags != nullptr && TargetTags->HasTagExact(GPTags::State::Status::Enemy::CrystalGuarded);
    const bool bCapturedTargetWingCoreExposed = TargetTags != nullptr && TargetTags->HasTagExact(GPTags::State::Status::Enemy::WingCoreExposed);
    const bool bTargetGroggy = bCapturedTargetGroggy
        || (TargetASC != nullptr && TargetASC->HasMatchingGameplayTag(GPTags::State::Status::Enemy::Groggy));
    const bool bTargetMatadorGuarded = bCapturedTargetMatadorGuarded
        || (TargetASC != nullptr && TargetASC->HasMatchingGameplayTag(GPTags::State::Status::Enemy::MatadorGuarded));
    const bool bTargetCrystalGuarded = bCapturedTargetCrystalGuarded
        || (TargetASC != nullptr && TargetASC->HasMatchingGameplayTag(GPTags::State::Status::Enemy::CrystalGuarded));
    const bool bTargetWingCoreExposed = bCapturedTargetWingCoreExposed
        || (TargetASC != nullptr && TargetASC->HasMatchingGameplayTag(GPTags::State::Status::Enemy::WingCoreExposed));
    float BossStateDamageMultiplier = 1.0f;
    if (!bTargetGroggy)
    {
        if (bTargetWingCoreExposed)
        {
            BossStateDamageMultiplier = GPDamageExec::CrystalWingCoreExposedFinalDamageMultiplier;
        }
        else if (bTargetCrystalGuarded)
        {
            BossStateDamageMultiplier = GPDamageExec::CrystalGuardedFinalDamageMultiplier;
        }
        else if (bTargetMatadorGuarded)
        {
            BossStateDamageMultiplier = GPDamageExec::MatadorGuardedFinalDamageMultiplier;
        }
    }
    Damage_Final *= BossStateDamageMultiplier;

	// Dark Knight owns directional guard/parry rules; the shared execution only supplies authoritative source/target context.
	float DarkKnightStateDamageMultiplier = 1.0f;
	if (TargetASC != nullptr)
	{
		AActor* TargetAvatar = TargetASC->GetAvatarActor();
		if (UGP_DarkArmorKnightStateComponent* DarkKnightState = IsValid(TargetAvatar)
			? TargetAvatar->FindComponentByClass<UGP_DarkArmorKnightStateComponent>()
			: nullptr)
		{
			const UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
			AActor* DamageInstigator = SourceASC != nullptr ? SourceASC->GetAvatarActor() : nullptr;
			const bool bHeavyAttack = Spec.GetDynamicAssetTags().HasTagExact(GPTags::Damage::Element::Brute);
			DarkKnightStateDamageMultiplier = DarkKnightState->ResolveIncomingDamageMultiplier(DamageInstigator, bHeavyAttack);
			Damage_Final *= DarkKnightStateDamageMultiplier;
		}
	}

    if (CVarGPDamageExecLog.GetValueOnAnyThread() != 0)
    {
        UE_LOG(LogTemp, Log, TEXT("[DamageExec] Element=%s SkillMult=%.2f Base=%.2f Critical=%s CritMult=%.2f Modified=%.2f Armor=%.2f ArmorMitigation=%.3f Resistance=%.3f MatadorGuarded=%d CrystalGuarded=%d WingCoreExposed=%d TargetGroggy=%d BossStateMult=%.2f DarkKnightMult=%.2f Final=%.2f"),
            ElementName,
            SkillDamageMultiplier,
            BaseDamage,
            bCritical ? TEXT("true") : TEXT("false"),
            FinalCritMult,
            Damage_Modified,
            Armor,
            ArmorMitigation,
            Resistance_Elem,
            bTargetMatadorGuarded ? 1 : 0,
            bTargetCrystalGuarded ? 1 : 0,
            bTargetWingCoreExposed ? 1 : 0,
            bTargetGroggy ? 1 : 0,
            BossStateDamageMultiplier,
			DarkKnightStateDamageMultiplier,
            Damage_Final);
    }

    // 5. 마. 강인도 피해 산출
	float ToughnessDamageBase = Spec.GetSetByCallerMagnitude(GPTags::Damage::Data::ToughnessBase, false, 0.0f);
	if (DarkKnightStateDamageMultiplier <= KINDA_SMALL_NUMBER)
	{
		// A successful parry cancels both health and toughness damage from that hit.
		ToughnessDamageBase = 0.0f;
	}

    OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UGP_AttributeSet::GetDamageAttribute(), EGameplayModOp::Additive, Damage_Final));
    OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UGP_AttributeSet::GetToughnessDamageAttribute(), EGameplayModOp::Additive, ToughnessDamageBase));
}
