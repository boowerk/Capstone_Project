#include "AbilitySystem/GP_AttributeSet.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"
#include "GameplayTags/GP_Tags.h"

void UGP_AttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Mana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MaxMana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, AttackPower, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MagicPower, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, AttackSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, CriticalChance, COND_None, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Lifesteal, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, CritMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, DamageIncreaseRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Armor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, PyrosResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, HydroResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, VoltResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, AeroResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, LuxResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, ChaosResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, BruteResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Toughness, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MaxToughness, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, ToughnessRecoveryRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MoveSpeed, COND_None, REPNOTIFY_Always);
	
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, bAttributesInitialized, COND_None, REPNOTIFY_Always);
}

void UGP_AttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	if (!bAttributesInitialized)
	{
		bAttributesInitialized = true;
		OnAttributesInitialized.Broadcast();
	}

	UAbilitySystemComponent* TargetASC = GetOwningAbilitySystemComponent();

	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		float LocalDamageDone = GetDamage();
		SetDamage(0.f);

		if (LocalDamageDone > 0.0f)
		{
			// 1. 체력 차감 로직
			const float NewHealth = FMath::Clamp(GetHealth() - LocalDamageDone, 0.0f, GetMaxHealth());
			SetHealth(NewHealth);

			// 2. 공격자(Instigator)와 타겟 정보 가져오기
			AActor* TargetActor = GetOwningActor();
			AActor* InstigatorActor = Data.EffectSpec.GetContext().GetOriginalInstigator();
			UAbilitySystemComponent* SourceASC = Data.EffectSpec.GetContext().GetOriginalInstigatorAbilitySystemComponent();

			FGameplayTag ElementTag = FGameplayTag::EmptyTag;
			if (IsValid(SourceASC))
			{
				// 2. 가. 신규 로직: 소스의 태그를 검사하여 ElementTag 결정
				if (SourceASC->HasMatchingGameplayTag(GPTags::Damage::Element::Hydro))
					ElementTag = GPTags::Damage::Element::Hydro;
				else if (SourceASC->HasMatchingGameplayTag(GPTags::Damage::Element::Volt))
					ElementTag = GPTags::Damage::Element::Volt;
				else if (SourceASC->HasMatchingGameplayTag(GPTags::Damage::Element::Aero))
					ElementTag = GPTags::Damage::Element::Aero;
				else if (SourceASC->HasMatchingGameplayTag(GPTags::Damage::Element::Lux))
					ElementTag = GPTags::Damage::Element::Lux;
				else if (SourceASC->HasMatchingGameplayTag(GPTags::Damage::Element::Chaos))
					ElementTag = GPTags::Damage::Element::Chaos;
				else if (SourceASC->HasMatchingGameplayTag(GPTags::Damage::Element::Brute))
					ElementTag = GPTags::Damage::Element::Brute;
				else
					ElementTag = GPTags::Damage::Element::Pyros; 

				// 3. 신규 기능: 흡혈 로직
				if (SourceASC != TargetASC)
				{
					if (SourceASC->HasAttributeSetForAttribute(UGP_AttributeSet::GetLifestealAttribute()))
					{
						float SourceLifesteal = SourceASC->GetNumericAttribute(UGP_AttributeSet::GetLifestealAttribute());
						if (SourceLifesteal > 0.0f)
						{
							float HealAmount = LocalDamageDone * SourceLifesteal;
							SourceASC->ApplyModToAttributeUnsafe(UGP_AttributeSet::GetHealthAttribute(), EGameplayModOp::Additive, HealAmount);
						}
					}
				}
			}

			// 4. UI 및 데미지 처리를 위한 델리게이트 방송 (기존 유지)
			OnDamageTaken.Broadcast(InstigatorActor, TargetActor, LocalDamageDone, ElementTag);
		}
	}

	// 신규 기능: 강인도 데미지 처리
	if (Data.EvaluatedData.Attribute == GetToughnessDamageAttribute())
	{
		float LocalToughnessDamage = GetToughnessDamage();
		SetToughnessDamage(0.0f);

		if (LocalToughnessDamage > 0.0f)
		{
			float OldToughness = GetToughness();
			float NewToughness = FMath::Clamp(OldToughness - LocalToughnessDamage, 0.0f, GetMaxToughness());
			SetToughness(NewToughness);

			// 강인도 고갈 시 Stun 상태 태그 부여 (기존 GPTags 사용)
			if (OldToughness > 0.0f && NewToughness <= 0.0f)
			{
				if (TargetASC)
				{
					TargetASC->AddLooseGameplayTag(GPTags::State::Debuff::Stun);
				}
			}
		}
	}
}

void UGP_AttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);
	ClampAttributeValue(Attribute, NewValue);
}

void UGP_AttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	ClampAttributeValue(Attribute, NewValue);
}

void UGP_AttributeSet::OnRep_AttributesInitialized()
{
	if (bAttributesInitialized)
	{
		// Replication applies the bool before OnRep runs, so broadcast when the received state is initialized.
		OnAttributesInitialized.Broadcast();
	}
}

void UGP_AttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue) const { GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Health, OldValue); }
void UGP_AttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue) const { GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, MaxHealth, OldValue); }
void UGP_AttributeSet::OnRep_Mana(const FGameplayAttributeData& OldValue) const { GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Mana, OldValue); }
void UGP_AttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldValue) const { GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, MaxMana, OldValue); }
void UGP_AttributeSet::OnRep_AttackPower(const FGameplayAttributeData& OldValue) const { GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, AttackPower, OldValue); }
void UGP_AttributeSet::OnRep_MagicPower(const FGameplayAttributeData& OldValue) const { GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, MagicPower, OldValue); }
void UGP_AttributeSet::OnRep_AttackSpeed(const FGameplayAttributeData& OldValue) const { GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, AttackSpeed, OldValue); }
void UGP_AttributeSet::OnRep_CriticalChance(const FGameplayAttributeData& OldValue) const { GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, CriticalChance, OldValue); }

void UGP_AttributeSet::OnRep_Lifesteal(const FGameplayAttributeData& OldValue) const { GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Lifesteal, OldValue); }
void UGP_AttributeSet::OnRep_CritMultiplier(const FGameplayAttributeData& OldValue) const { GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, CritMultiplier, OldValue); }
void UGP_AttributeSet::OnRep_DamageIncreaseRate(const FGameplayAttributeData& OldValue) const { GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, DamageIncreaseRate, OldValue); }
void UGP_AttributeSet::OnRep_Armor(const FGameplayAttributeData& OldValue) const { GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Armor, OldValue); }
void UGP_AttributeSet::OnRep_PyrosResistance(const FGameplayAttributeData& OldValue) const { GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, PyrosResistance, OldValue); }
void UGP_AttributeSet::OnRep_HydroResistance(const FGameplayAttributeData& OldValue) const { GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, HydroResistance, OldValue); }
void UGP_AttributeSet::OnRep_VoltResistance(const FGameplayAttributeData& OldValue) const { GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, VoltResistance, OldValue); }
void UGP_AttributeSet::OnRep_AeroResistance(const FGameplayAttributeData& OldValue) const { GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, AeroResistance, OldValue); }
void UGP_AttributeSet::OnRep_LuxResistance(const FGameplayAttributeData& OldValue) const { GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, LuxResistance, OldValue); }
void UGP_AttributeSet::OnRep_ChaosResistance(const FGameplayAttributeData& OldValue) const { GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, ChaosResistance, OldValue); }
void UGP_AttributeSet::OnRep_BruteResistance(const FGameplayAttributeData& OldValue) const { GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, BruteResistance, OldValue); }
void UGP_AttributeSet::OnRep_Toughness(const FGameplayAttributeData& OldValue) const { GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Toughness, OldValue); }
void UGP_AttributeSet::OnRep_MaxToughness(const FGameplayAttributeData& OldValue) const { GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, MaxToughness, OldValue); }
void UGP_AttributeSet::OnRep_ToughnessRecoveryRate(const FGameplayAttributeData& OldValue) const { GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, ToughnessRecoveryRate, OldValue); }
void UGP_AttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldValue) const { GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, MoveSpeed, OldValue); }

void UGP_AttributeSet::ClampAttributeValue(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetAttackPowerAttribute() || Attribute == GetMagicPowerAttribute())
	{
		NewValue = FMath::Max(0.0f, NewValue);
		return;
	}
	if (Attribute == GetAttackSpeedAttribute())
	{
		NewValue = FMath::Max(0.1f, NewValue);
		return;
	}
	if (Attribute == GetCriticalChanceAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, 1.0f);
		return;
	}
	if (Attribute == GetHealthAttribute())
	{
		const float MaxH = GetMaxHealth();
		NewValue = (MaxH > 0.0f) ? FMath::Clamp(NewValue, 0.0f, MaxH) : FMath::Max(0.0f, NewValue);
		return;
	}
	if (Attribute == GetToughnessAttribute())
	{
		const float MaxT = GetMaxToughness();
		NewValue = (MaxT > 0.0f) ? FMath::Clamp(NewValue, 0.0f, MaxT) : FMath::Max(0.0f, NewValue);
		return;
	}
}
