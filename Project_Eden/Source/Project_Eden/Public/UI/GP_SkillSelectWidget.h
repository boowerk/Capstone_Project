#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"

#include "GP_SkillSelectWidget.generated.h"

class AGP_PlayerController;
class AGP_PlayerState;
class UGP_SkillData;

UCLASS(Abstract, Blueprintable)
class PROJECT_EDEN_API UGP_SkillSelectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Skill Selection")
	void RefreshSkillSelection();

	UFUNCTION(BlueprintPure, Category = "Skill Selection")
	TArray<UGP_SkillData*> GetAvailableSkills() const;

	UFUNCTION(BlueprintCallable, Category = "Skill Selection")
	void SelectSlot01();

	UFUNCTION(BlueprintCallable, Category = "Skill Selection")
	void SelectSlot02();

	UFUNCTION(BlueprintCallable, Category = "Skill Selection")
	bool EquipSkillToSelectedSlot(UGP_SkillData* SkillData);

	UFUNCTION(BlueprintCallable, Category = "Skill Selection")
	bool EquipSkillToSlot(UGP_SkillData* SkillData, FGameplayTag SlotTag);

	UFUNCTION(BlueprintPure, Category = "Skill Selection")
	UGP_SkillData* GetEquippedSkill(FGameplayTag SlotTag) const;

	UFUNCTION(BlueprintPure, Category = "Skill Selection")
	FGameplayTag GetSelectedSlotTag() const { return SelectedSlotTag; }

	UFUNCTION(BlueprintPure, Category = "Skill Selection")
	static FGameplayTag GetSlot01Tag();

	UFUNCTION(BlueprintPure, Category = "Skill Selection")
	static FGameplayTag GetSlot02Tag();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Skill Selection", meta = (DisplayName = "On Skill Selection Refreshed"))
	void BP_OnSkillSelectionRefreshed(
		const TArray<UGP_SkillData*>& AvailableSkills,
		UGP_SkillData* Slot01Skill,
		UGP_SkillData* Slot02Skill,
		FGameplayTag ActiveSlotTag);

private:
	UFUNCTION()
	void HandleEquippedSkillChanged(FGameplayTag SlotTag, UGP_SkillData* SkillData);

	AGP_PlayerController* GetGPPlayerController() const;
	AGP_PlayerState* GetGPPlayerState() const;
	void SetSelectedSlot(FGameplayTag SlotTag);

	UPROPERTY(Transient)
	FGameplayTag SelectedSlotTag;
};
