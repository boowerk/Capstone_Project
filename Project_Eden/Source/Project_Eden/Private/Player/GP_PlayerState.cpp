#include "Player/GP_PlayerState.h"
#include "AbilitySystem/GP_AbilitySystemComponent.h"
#include "AbilitySystem/GP_WeaponAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystem/GP_AttributeSet.h"


AGP_PlayerState::AGP_PlayerState()
{
	SetNetUpdateFrequency(100.f);

	AbilitySystemComponent = CreateDefaultSubobject<UGP_AbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UGP_AttributeSet>("AttributeSet");
	
	WeaponAttributeSet = CreateDefaultSubobject<UGP_WeaponAttributeSet>("WeaponAttributeSet");//
}

UAbilitySystemComponent* AGP_PlayerState::GetAbilitySystemComponent() const {
	return AbilitySystemComponent;
}

bool AGP_PlayerState::EquipWeaponFromCollection(UPDA_WeaponItemCollection* WeaponCollection, FName WeaponId)
{
	if (!HasAuthority() || !IsValid(AbilitySystemComponent) || !IsValid(WeaponCollection) || WeaponId.IsNone())
	{
		return false;
	}

	FWeaponItemData WeaponData;
	if (!WeaponCollection->GetWeaponDataById(WeaponId, WeaponData))
	{
		return false;
	}

	EquippedWeaponId = WeaponData.ItemId;
	EquippedWeaponElement = WeaponData.Element;
	EquippedWeaponRarity = WeaponData.Rarity;

	AbilitySystemComponent->SetNumericAttributeBase(UGP_WeaponAttributeSet::GetAttackPowerAttribute(), static_cast<float>(WeaponData.AttackPower));
	AbilitySystemComponent->SetNumericAttributeBase(UGP_WeaponAttributeSet::GetMagicPowerAttribute(), static_cast<float>(WeaponData.MagicPower));
	AbilitySystemComponent->SetNumericAttributeBase(UGP_WeaponAttributeSet::GetAttackSpeedAttribute(), WeaponData.AttackSpeed);
	AbilitySystemComponent->SetNumericAttributeBase(UGP_WeaponAttributeSet::GetCriticalChanceAttribute(), WeaponData.CriticalChance);

	ForceNetUpdate();
	return true;
}

void AGP_PlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGP_PlayerState, EquippedWeaponId);
	DOREPLIFETIME(AGP_PlayerState, EquippedWeaponElement);
	DOREPLIFETIME(AGP_PlayerState, EquippedWeaponRarity);
}

void AGP_PlayerState::OnRep_EquippedWeaponData()
{
}
