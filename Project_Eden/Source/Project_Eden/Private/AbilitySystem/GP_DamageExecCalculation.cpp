#include "AbilitySystem/GP_DamageExecCalculation.h"
#include "AbilitySystem/GP_AttributeSet.h"
#include "GameplayTags/GP_Tags.h"
#include "AbilitySystemComponent.h"
#include "Math/UnrealMathUtility.h"

struct FGP_DamageStatics
{
    DECLARE_ATTRIBUTE_CAPTUREDEF(AttackPower);
    DECLARE_ATTRIBUTE_CAPTUREDEF(MagicPower);
    DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalChance);
    DECLARE_ATTRIBUTE_CAPTUREDEF(CritMultiplier);
    DECLARE_ATTRIBUTE_CAPTUREDEF(DamageIncreaseRate);

    DECLARE_ATTRIBUTE_CAPTUREDEF(Armor);
    DECLARE_ATTRIBUTE_CAPTUREDEF(MaxHealth);
    
    DECLARE_ATTRIBUTE_CAPTUREDEF(FireResistance);
    DECLARE_ATTRIBUTE_CAPTUREDEF(WaterResistance);
    DECLARE_ATTRIBUTE_CAPTUREDEF(ElectricityResistance);
    DECLARE_ATTRIBUTE_CAPTUREDEF(IceResistance);
    DECLARE_ATTRIBUTE_CAPTUREDEF(PoisonResistance);
    DECLARE_ATTRIBUTE_CAPTUREDEF(LightResistance);

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
        
        DEFINE_ATTRIBUTE_CAPTUREDEF(UGP_AttributeSet, FireResistance, Target, false);
        DEFINE_ATTRIBUTE_CAPTUREDEF(UGP_AttributeSet, WaterResistance, Target, false);
        DEFINE_ATTRIBUTE_CAPTUREDEF(UGP_AttributeSet, ElectricityResistance, Target, false);
        DEFINE_ATTRIBUTE_CAPTUREDEF(UGP_AttributeSet, IceResistance, Target, false);
        DEFINE_ATTRIBUTE_CAPTUREDEF(UGP_AttributeSet, PoisonResistance, Target, false);
        DEFINE_ATTRIBUTE_CAPTUREDEF(UGP_AttributeSet, LightResistance, Target, false);
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

    RelevantAttributesToCapture.Add(DamageStatics().FireResistanceDef);
    RelevantAttributesToCapture.Add(DamageStatics().WaterResistanceDef);
    RelevantAttributesToCapture.Add(DamageStatics().ElectricityResistanceDef);
    RelevantAttributesToCapture.Add(DamageStatics().IceResistanceDef);
    RelevantAttributesToCapture.Add(DamageStatics().PoisonResistanceDef);
    RelevantAttributesToCapture.Add(DamageStatics().LightResistanceDef);
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
    float C_atk = Spec.GetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Damage.Coef.Atk")), false, 1.0f);
    float C_def = Spec.GetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Damage.Coef.Def")), false, 0.0f);
    float C_hp  = Spec.GetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Damage.Coef.Hp")), false, 0.0f);
    
    // [기존/신규 로직 통합] 물리/마법 분기
    if (Spec.GetDynamicAssetTags().HasTagExact(GPTags::Damage::Type::Magical))
    {
        float M_atk = Spec.GetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Damage.Coef.M_Atk")), false, 1.0f);
        float BaseSpell = Spec.GetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Damage.BaseSpell")), false, 0.0f);
        // 마법력(MagicPower) 우선 참조
        BaseDamage = (MagicPower * M_atk) + (AttackPower * 0.2f) + BaseSpell; 
    }
    else
    {
        // 물리 공식 적용
        BaseDamage = (AttackPower * C_atk) + (Armor * C_def) + (MaxHealth * C_hp); 
    }

    // 3. 나. 공격 수정치 적용 (치명타 및 피해 증가)
    bool bCritical = FMath::RandRange(0.0f, 1.0f) <= CriticalChance;
    // CritMultiplier 기본값은 1.5로 설정 (기획안)
    float FinalCritMult = bCritical ? FMath::Max(CritMultiplier, 1.5f) : 1.0f;
    
    float Damage_Modified = BaseDamage * FinalCritMult * (1.0f + DamageIncreaseRate);

    // 4. 다/라. 속성 및 방어력 경감
    float Resistance_Elem = 0.0f;
    if (Spec.GetDynamicAssetTags().HasTagExact(GPTags::Damage::Element::Fire)) { ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().FireResistanceDef, EvaluationParameters, Resistance_Elem); }
    else if (Spec.GetDynamicAssetTags().HasTagExact(GPTags::Damage::Element::Water)) { ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().WaterResistanceDef, EvaluationParameters, Resistance_Elem); }
    else if (Spec.GetDynamicAssetTags().HasTagExact(GPTags::Damage::Element::Electricity)) { ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().ElectricityResistanceDef, EvaluationParameters, Resistance_Elem); }
    else if (Spec.GetDynamicAssetTags().HasTagExact(GPTags::Damage::Element::Ice)) { ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().IceResistanceDef, EvaluationParameters, Resistance_Elem); }
    else if (Spec.GetDynamicAssetTags().HasTagExact(GPTags::Damage::Element::Poison)) { ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().PoisonResistanceDef, EvaluationParameters, Resistance_Elem); }
    else if (Spec.GetDynamicAssetTags().HasTagExact(GPTags::Damage::Element::Light)) { ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().LightResistanceDef, EvaluationParameters, Resistance_Elem); }

    float K = 100.0f; 
    float ArmorMitigation = K / (K + FMath::Max(Armor, 0.0f));
    
    float Damage_Final = Damage_Modified * ArmorMitigation * (1.0f - Resistance_Elem);

    // 5. 마. 강인도 피해 산출
    float ToughnessDamageBase = Spec.GetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Damage.ToughnessBase")), false, 0.0f);

    OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UGP_AttributeSet::GetDamageAttribute(), EGameplayModOp::Additive, Damage_Final));
    OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UGP_AttributeSet::GetToughnessDamageAttribute(), EGameplayModOp::Additive, ToughnessDamageBase));
}