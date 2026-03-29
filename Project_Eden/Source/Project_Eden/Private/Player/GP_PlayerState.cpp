#include "Player/GP_PlayerState.h"
#include "AbilitySystem/GP_AbilitySystemComponent.h"
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
	

	ForceNetUpdate();
	return true;
}

void AGP_PlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
}

void AGP_PlayerState::OnRep_EquippedWeaponData()
{
}
