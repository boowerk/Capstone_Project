#include "AbilitySystem/GP_AttributeSet.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

void UGP_AttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 기존 UGP_AttributeSet 속성들
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Mana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MaxMana, COND_None, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, AttackPower, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MagicPower, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, AttackSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, CriticalChance, COND_None, REPNOTIFY_Always);
	
	DOREPLIFETIME(ThisClass, bAttributesInitialized);
}

void UGP_AttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	if (!bAttributesInitialized)
	{
		bAttributesInitialized = true;
		OnAttributesInitialized.Broadcast();
	}
	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		float LocalDamageDone = GetDamage();
		SetDamage(0.f);

		if (LocalDamageDone > 0.0f)
		{
			// 1. 체력 차감 로직 (수학적 계산만 수행)
			const float NewHealth = FMath::Clamp(GetHealth() - LocalDamageDone, 0.0f, GetMaxHealth());
			SetHealth(NewHealth);

			// 2. 공격자(Instigator)와 타겟 정보 가져오기
			AActor* TargetActor = GetOwningActor();
			AActor* InstigatorActor = Data.EffectSpec.GetContext().GetOriginalInstigator();
			UAbilitySystemComponent* SourceASC = Data.EffectSpec.GetContext().GetOriginalInstigatorAbilitySystemComponent();

			FGameplayTag ElementTag = FGameplayTag::EmptyTag;
			if (IsValid(SourceASC))
			{
				if (SourceASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("Weapon.Element.Water"), false)))
					ElementTag = FGameplayTag::RequestGameplayTag(FName("Weapon.Element.Water"), false);
				else if (SourceASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("Weapon.Element.Lightning"), false)))
					ElementTag = FGameplayTag::RequestGameplayTag(FName("Weapon.Element.Lightning"), false);
				else
					ElementTag = FGameplayTag::RequestGameplayTag(FName("Weapon.Element.Fire"), false); 
			}

			// 4. UI 호출 대신 델리게이트 방송
			OnDamageTaken.Broadcast(InstigatorActor, TargetActor, LocalDamageDone, ElementTag);
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
	if (!bAttributesInitialized)
	{
		bAttributesInitialized = true;
		OnAttributesInitialized.Broadcast();
	}
}

void UGP_AttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Health, OldValue);
}

void UGP_AttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, MaxHealth, OldValue);
}

void UGP_AttributeSet::OnRep_Mana(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Mana, OldValue);
}

void UGP_AttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, MaxMana, OldValue);
}

void UGP_AttributeSet::OnRep_AttackPower(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, AttackPower, OldValue);
}

void UGP_AttributeSet::OnRep_MagicPower(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, MagicPower, OldValue);
}

void UGP_AttributeSet::OnRep_AttackSpeed(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, AttackSpeed, OldValue);
}

void UGP_AttributeSet::OnRep_CriticalChance(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, CriticalChance, OldValue);
}


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
	}
}
