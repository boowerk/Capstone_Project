#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"

#include "GP_WeaponAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class PROJECT_EDEN_API UGP_WeaponAttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    UGP_WeaponAttributeSet();

    ATTRIBUTE_ACCESSORS(UGP_WeaponAttributeSet, AttackPower)
    ATTRIBUTE_ACCESSORS(UGP_WeaponAttributeSet, MagicPower)
    ATTRIBUTE_ACCESSORS(UGP_WeaponAttributeSet, AttackSpeed)
    ATTRIBUTE_ACCESSORS(UGP_WeaponAttributeSet, CriticalChance)

    virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AttackPower, Category = "Weapon")
    FGameplayAttributeData AttackPower;

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MagicPower, Category = "Weapon")
    FGameplayAttributeData MagicPower;

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AttackSpeed, Category = "Weapon")
    FGameplayAttributeData AttackSpeed;

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CriticalChance, Category = "Weapon")
    FGameplayAttributeData CriticalChance;

    UFUNCTION()
    void OnRep_AttackPower(const FGameplayAttributeData& OldValue) const;

    UFUNCTION()
    void OnRep_MagicPower(const FGameplayAttributeData& OldValue) const;

    UFUNCTION()
    void OnRep_AttackSpeed(const FGameplayAttributeData& OldValue) const;

    UFUNCTION()
    void OnRep_CriticalChance(const FGameplayAttributeData& OldValue) const;

private:
    void ClampAttributeValue(const FGameplayAttribute& Attribute, float& NewValue) const;
};
