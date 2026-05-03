#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"

#include "GP_AttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnDamageTaken, AActor*, Instigator, AActor*, Target, float, DamageAmount, FGameplayTag, ElementTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAttributesInitialized);

UCLASS()
class PROJECT_EDEN_API UGP_AttributeSet : public UAttributeSet
{
	GENERATED_BODY()
public:
	using ThisClass = UGP_AttributeSet;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	
	UPROPERTY(BlueprintAssignable)
	FAttributesInitialized OnAttributesInitialized;

	UPROPERTY(BlueprintAssignable, Category = "GAS|Events")
	FOnDamageTaken OnDamageTaken;
	
	UPROPERTY(ReplicatedUsing = OnRep_AttributesInitialized)
	bool bAttributesInitialized = false;

	UFUNCTION()
	void OnRep_AttributesInitialized();
	
	// -------------------------------------------------------------------
	// [A] 기존 속성들 - 순서 절대 유지 (직렬화 오프셋 보호)
	// -------------------------------------------------------------------
	UPROPERTY(BlueprintReadOnly, Category = "Meta")
	FGameplayAttributeData Damage;
	ATTRIBUTE_ACCESSORS(ThisClass, Damage);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(ThisClass, Health);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(ThisClass, MaxHealth);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Mana)
	FGameplayAttributeData Mana;
	ATTRIBUTE_ACCESSORS(ThisClass, Mana);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxMana)
	FGameplayAttributeData MaxMana;
	ATTRIBUTE_ACCESSORS(ThisClass, MaxMana);
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AttackPower, Category = "Weapon")
	FGameplayAttributeData AttackPower;
	ATTRIBUTE_ACCESSORS(ThisClass, AttackPower);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MagicPower, Category = "Weapon")
	FGameplayAttributeData MagicPower;
	ATTRIBUTE_ACCESSORS(ThisClass, MagicPower);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AttackSpeed, Category = "Weapon")
	FGameplayAttributeData AttackSpeed;
	ATTRIBUTE_ACCESSORS(ThisClass, AttackSpeed);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CriticalChance, Category = "Weapon")
	FGameplayAttributeData CriticalChance;
	ATTRIBUTE_ACCESSORS(ThisClass, CriticalChance);

	// -------------------------------------------------------------------
	// [B] 신규 추가 속성들 - 하단 배치
	// -------------------------------------------------------------------
	UPROPERTY(BlueprintReadOnly, Category = "Meta")
	FGameplayAttributeData ToughnessDamage;
	ATTRIBUTE_ACCESSORS(ThisClass, ToughnessDamage);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Lifesteal, Category = "Vitality")
	FGameplayAttributeData Lifesteal;
	ATTRIBUTE_ACCESSORS(ThisClass, Lifesteal);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CritMultiplier, Category = "Weapon")
	FGameplayAttributeData CritMultiplier;
	ATTRIBUTE_ACCESSORS(ThisClass, CritMultiplier);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_DamageIncreaseRate, Category = "Weapon")
	FGameplayAttributeData DamageIncreaseRate;
	ATTRIBUTE_ACCESSORS(ThisClass, DamageIncreaseRate);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Armor, Category = "Defensive")
	FGameplayAttributeData Armor;
	ATTRIBUTE_ACCESSORS(ThisClass, Armor);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_FireResistance, Category = "Defensive")
	FGameplayAttributeData FireResistance;
	ATTRIBUTE_ACCESSORS(ThisClass, FireResistance);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_WaterResistance, Category = "Defensive")
	FGameplayAttributeData WaterResistance;
	ATTRIBUTE_ACCESSORS(ThisClass, WaterResistance);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ElectricityResistance, Category = "Defensive")     
	FGameplayAttributeData ElectricityResistance;
	ATTRIBUTE_ACCESSORS(ThisClass, ElectricityResistance);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_IceResistance, Category = "Defensive")
	FGameplayAttributeData IceResistance;
	ATTRIBUTE_ACCESSORS(ThisClass, IceResistance);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PoisonResistance, Category = "Defensive")
	FGameplayAttributeData PoisonResistance;
	ATTRIBUTE_ACCESSORS(ThisClass, PoisonResistance);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_LightResistance, Category = "Defensive")
	FGameplayAttributeData LightResistance;
	ATTRIBUTE_ACCESSORS(ThisClass, LightResistance);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Toughness, Category = "Toughness")
	FGameplayAttributeData Toughness;
	ATTRIBUTE_ACCESSORS(ThisClass, Toughness);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxToughness, Category = "Toughness")
	FGameplayAttributeData MaxToughness;
	ATTRIBUTE_ACCESSORS(ThisClass, MaxToughness);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ToughnessRecoveryRate, Category = "Toughness")     
	FGameplayAttributeData ToughnessRecoveryRate;
	ATTRIBUTE_ACCESSORS(ThisClass, ToughnessRecoveryRate);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MoveSpeed, Category = "Utility")
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS(ThisClass, MoveSpeed);

	// -------------------------------------------------------------------
	// OnRep Functions
	// -------------------------------------------------------------------
	UFUNCTION() void OnRep_Health(const FGameplayAttributeData& OldValue) const;
	UFUNCTION() void OnRep_MaxHealth(const FGameplayAttributeData& OldValue) const;
	UFUNCTION() void OnRep_Mana(const FGameplayAttributeData& OldValue) const;
	UFUNCTION() void OnRep_MaxMana(const FGameplayAttributeData& OldValue) const;
	UFUNCTION() void OnRep_AttackPower(const FGameplayAttributeData& OldValue) const;
	UFUNCTION() void OnRep_MagicPower(const FGameplayAttributeData& OldValue) const;
	UFUNCTION() void OnRep_AttackSpeed(const FGameplayAttributeData& OldValue) const;
	UFUNCTION() void OnRep_CriticalChance(const FGameplayAttributeData& OldValue) const;
	
	UFUNCTION() void OnRep_Lifesteal(const FGameplayAttributeData& OldValue) const;
	UFUNCTION() void OnRep_CritMultiplier(const FGameplayAttributeData& OldValue) const;
	UFUNCTION() void OnRep_DamageIncreaseRate(const FGameplayAttributeData& OldValue) const;
	UFUNCTION() void OnRep_Armor(const FGameplayAttributeData& OldValue) const;
	UFUNCTION() void OnRep_FireResistance(const FGameplayAttributeData& OldValue) const;
	UFUNCTION() void OnRep_WaterResistance(const FGameplayAttributeData& OldValue) const;
	UFUNCTION() void OnRep_ElectricityResistance(const FGameplayAttributeData& OldValue) const;
	UFUNCTION() void OnRep_IceResistance(const FGameplayAttributeData& OldValue) const;
	UFUNCTION() void OnRep_PoisonResistance(const FGameplayAttributeData& OldValue) const;
	UFUNCTION() void OnRep_LightResistance(const FGameplayAttributeData& OldValue) const;
	UFUNCTION() void OnRep_Toughness(const FGameplayAttributeData& OldValue) const;
	UFUNCTION() void OnRep_MaxToughness(const FGameplayAttributeData& OldValue) const;
	UFUNCTION() void OnRep_ToughnessRecoveryRate(const FGameplayAttributeData& OldValue) const;
	UFUNCTION() void OnRep_MoveSpeed(const FGameplayAttributeData& OldValue) const;

private:
	void ClampAttributeValue(const FGameplayAttribute& Attribute, float& NewValue) const;
};
