#include "AbilitySystem/GP_WeaponAttributeSet.h"

#include "Net/UnrealNetwork.h"

UGP_WeaponAttributeSet::UGP_WeaponAttributeSet()
{
    InitAttackPower(0.0f);
    InitMagicPower(0.0f);
    InitAttackSpeed(1.0f);
    InitCriticalChance(0.0f);
}

void UGP_WeaponAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
    Super::PreAttributeBaseChange(Attribute, NewValue);
    ClampAttributeValue(Attribute, NewValue);
}

void UGP_WeaponAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);
    ClampAttributeValue(Attribute, NewValue);
}

void UGP_WeaponAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UGP_WeaponAttributeSet, AttackPower, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UGP_WeaponAttributeSet, MagicPower, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UGP_WeaponAttributeSet, AttackSpeed, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UGP_WeaponAttributeSet, CriticalChance, COND_None, REPNOTIFY_Always);
}

void UGP_WeaponAttributeSet::OnRep_AttackPower(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UGP_WeaponAttributeSet, AttackPower, OldValue);
}

void UGP_WeaponAttributeSet::OnRep_MagicPower(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UGP_WeaponAttributeSet, MagicPower, OldValue);
}

void UGP_WeaponAttributeSet::OnRep_AttackSpeed(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UGP_WeaponAttributeSet, AttackSpeed, OldValue);
}

void UGP_WeaponAttributeSet::OnRep_CriticalChance(const FGameplayAttributeData& OldValue) const
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UGP_WeaponAttributeSet, CriticalChance, OldValue);
}

void UGP_WeaponAttributeSet::ClampAttributeValue(const FGameplayAttribute& Attribute, float& NewValue) const
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
