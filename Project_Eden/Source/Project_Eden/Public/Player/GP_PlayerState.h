#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "Items/WeaponItemTypes.h"

#include "GP_PlayerState.generated.h"

class UAbilitySystemComponent;
class UGP_WeaponAttributeSet;
class UPDA_WeaponItemCollection;

UCLASS()
class PROJECT_EDEN_API AGP_PlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	AGP_PlayerState();
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool EquipWeaponFromCollection(UPDA_WeaponItemCollection* WeaponCollection, FName WeaponId);

	UFUNCTION(BlueprintPure, Category = "Equipment")
	const UGP_WeaponAttributeSet* GetWeaponAttributeSet() const { return WeaponAttributeSet; }

	UFUNCTION(BlueprintPure, Category = "Equipment")
	FName GetEquippedWeaponId() const { return EquippedWeaponId; }

	UFUNCTION(BlueprintPure, Category = "Equipment")
	EWeaponElement GetEquippedWeaponElement() const { return EquippedWeaponElement; }

	UFUNCTION(BlueprintPure, Category = "Equipment")
	EWeaponRarity GetEquippedWeaponRarity() const { return EquippedWeaponRarity; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:

	UPROPERTY(VisibleAnywhere, Category = "GAS|Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, Category = "GAS|Attributes")
	TObjectPtr<UGP_WeaponAttributeSet> WeaponAttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_EquippedWeaponData, Category = "Equipment", meta = (AllowPrivateAccess = "true"))
	FName EquippedWeaponId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_EquippedWeaponData, Category = "Equipment", meta = (AllowPrivateAccess = "true"))
	EWeaponElement EquippedWeaponElement = EWeaponElement::Fire;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_EquippedWeaponData, Category = "Equipment", meta = (AllowPrivateAccess = "true"))
	EWeaponRarity EquippedWeaponRarity = EWeaponRarity::Common;

	UFUNCTION()
	void OnRep_EquippedWeaponData();
};
