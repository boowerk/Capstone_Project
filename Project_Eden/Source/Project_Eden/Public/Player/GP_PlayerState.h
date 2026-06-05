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
class UGP_SkillAugmentData;

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

	UFUNCTION(BlueprintCallable, Category = "Tech|Augment")
	void AddSkillAugment(UGP_SkillAugmentData* AugmentData);

	UFUNCTION(Server, Reliable)
	void ServerAddSkillAugment(UGP_SkillAugmentData* AugmentData);

	UFUNCTION(BlueprintPure, Category = "Tech|Augment")
	TArray<UGP_SkillAugmentData*> GetSelectedSkillAugments() const;

	UFUNCTION(BlueprintPure, Category = "Tech|Augment")
	float GetSkillAugmentDamageMultiplier(FGameplayTag SkillIdTag) const;

	UFUNCTION(BlueprintPure, Category = "Tech|Augment")
	float GetSkillAugmentRadiusMultiplier(FGameplayTag SkillIdTag) const;

	UFUNCTION(BlueprintPure, Category = "Tech|Augment")
	float GetSkillAugmentRangeMultiplier(FGameplayTag SkillIdTag) const;

	UFUNCTION(BlueprintPure, Category = "Tech|Augment")
	float GetSkillAugmentCooldownMultiplier(FGameplayTag SkillIdTag) const;

	UFUNCTION(BlueprintPure, Category = "Tech|Augment")
	int32 GetSkillAugmentProjectileCountBonus(FGameplayTag SkillIdTag) const;

	UFUNCTION(BlueprintCallable, Category = "Progression")
	void AddXP(float Amount);

	UFUNCTION(Server, Reliable)
	void ServerAddXP(float Amount);

	UFUNCTION(BlueprintPure, Category = "Progression")
	float GetCurrentXP() const { return CurrentXP; }

	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetCurrentLevel() const { return CurrentLevel; }

	UFUNCTION(BlueprintPure, Category = "Progression")
	float GetXPToNextLevel() const { return XPToNextLevel; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Progression")
	void OnLevelUp(int32 NewLevel);

private:

	// UGP_AbilitySystemComponent
	UPROPERTY(VisibleAnywhere, Category = "GAS|Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
	// UGP_AttributeSet
	UPROPERTY(VisibleAnywhere, Category = "GAS|Attributes")
	TObjectPtr<UAttributeSet> AttributeSet;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentTechElementTag, EditDefaultsOnly, BlueprintReadOnly, Category = "Tech", meta = (AllowPrivateAccess = "true", Categories = "GPTags.Tech.Element"))
	FGameplayTag CurrentTechElementTag;

	UPROPERTY(ReplicatedUsing = OnRep_SelectedSkillAugments, BlueprintReadOnly, Category = "Tech|Augment", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UGP_SkillAugmentData>> SelectedSkillAugments;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentXP, EditDefaultsOnly, BlueprintReadOnly, Category = "Progression", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float CurrentXP = 0.0f;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentLevel, EditDefaultsOnly, BlueprintReadOnly, Category = "Progression", meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 CurrentLevel = 1;

	UPROPERTY(ReplicatedUsing = OnRep_XPToNextLevel, EditDefaultsOnly, BlueprintReadOnly, Category = "Progression", meta = (AllowPrivateAccess = "true", ClampMin = "1.0"))
	float XPToNextLevel = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression", meta = (AllowPrivateAccess = "true", ClampMin = "1.0"))
	float LevelXPScale = 1.25f;

	UFUNCTION()
	void OnRep_EquippedWeaponData();

	UFUNCTION()
	void OnRep_CurrentTechElementTag();

	UFUNCTION()
	void OnRep_SelectedSkillAugments();

	UFUNCTION()
	void OnRep_CurrentXP();

	UFUNCTION()
	void OnRep_CurrentLevel(int32 PreviousLevel);

	UFUNCTION()
	void OnRep_XPToNextLevel();

	bool DoesAugmentApplyToSkill(const UGP_SkillAugmentData* AugmentData, FGameplayTag SkillIdTag) const;
};
