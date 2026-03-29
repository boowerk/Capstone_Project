#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "Items/WeaponItemTypes.h"

#include "GP_PlayerState.generated.h"

class UAttributeSet;
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
	
	UAttributeSet* GetAttributeSet() const { return AttributeSet; }

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool EquipWeaponFromCollection(UPDA_WeaponItemCollection* WeaponCollection, FName WeaponId);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:

	// UGP_AbilitySystemComponent
	UPROPERTY(VisibleAnywhere, Category = "GAS|Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
	// UGP_AttributeSet
	UPROPERTY(VisibleAnywhere, Category = "GAS|Attributes")
	TObjectPtr<UAttributeSet> AttributeSet;

	UFUNCTION()
	void OnRep_EquippedWeaponData();
};
