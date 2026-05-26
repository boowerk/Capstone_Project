#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
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

	UFUNCTION(BlueprintCallable, Category = "Tech")
	void SetCurrentTechElementTag(FGameplayTag NewElementTag);

	UFUNCTION(Server, Reliable)
	void ServerSetCurrentTechElementTag(FGameplayTag NewElementTag);

	UFUNCTION(BlueprintPure, Category = "Tech")
	FGameplayTag GetCurrentTechElementTag() const { return CurrentTechElementTag; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:

	// UGP_AbilitySystemComponent
	UPROPERTY(VisibleAnywhere, Category = "GAS|Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
	// UGP_AttributeSet
	UPROPERTY(VisibleAnywhere, Category = "GAS|Attributes")
	TObjectPtr<UAttributeSet> AttributeSet;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentTechElementTag, EditDefaultsOnly, BlueprintReadOnly, Category = "Tech", meta = (AllowPrivateAccess = "true", Categories = "GPTags.Tech.Element"))
	FGameplayTag CurrentTechElementTag;

	UFUNCTION()
	void OnRep_EquippedWeaponData();

	UFUNCTION()
	void OnRep_CurrentTechElementTag();
};
