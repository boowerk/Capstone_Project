#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"

#include "GP_SkillSelectWidget.generated.h"

class AGP_PlayerController;
class AGP_PlayerState;
class UGP_SkillData;
class UGP_SkillEntryWidget;
class UButton;
class UCanvasPanel;
class UImage;
class UTextBlock;
class UTexture2D;
class UVerticalBox;

UCLASS(Abstract, Blueprintable)
class PROJECT_EDEN_API UGP_SkillSelectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UGP_SkillSelectWidget(const FObjectInitializer& ObjectInitializer);

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

	UFUNCTION(BlueprintCallable, Category = "Skill Selection")
	bool EquipSelectedSkillToSlot(FGameplayTag SlotTag);

	UFUNCTION(BlueprintCallable, Category = "Skill Selection")
	void CloseSkillSelection();

	UFUNCTION(BlueprintPure, Category = "Skill Selection")
	UGP_SkillData* GetEquippedSkill(FGameplayTag SlotTag) const;

	UFUNCTION(BlueprintPure, Category = "Skill Selection")
	FGameplayTag GetSelectedSlotTag() const { return SelectedSlotTag; }

	UFUNCTION(BlueprintPure, Category = "Skill Selection")
	UGP_SkillData* GetSelectedSkill() const { return SelectedSkillData.Get(); }

	UFUNCTION(BlueprintPure, Category = "Skill Selection")
	static FGameplayTag GetSlot01Tag();

	UFUNCTION(BlueprintPure, Category = "Skill Selection")
	static FGameplayTag GetSlot02Tag();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Selection|Entries")
	TSubclassOf<UGP_SkillEntryWidget> SkillEntryWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Selection|Wheel", meta = (ClampMin = "0.0"))
	float SkillWheelRadius = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Selection|Wheel", meta = (ClampMin = "1.0"))
	float SkillWheelEntrySize = 112.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Selection|Wheel")
	float SkillWheelStartAngleDegrees = -90.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Selection|Fallback")
	TSoftObjectPtr<UTexture2D> RuntimeFallbackBackplateTexture;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Selection|Fallback")
	TSoftObjectPtr<UTexture2D> RuntimeFallbackRingTexture;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> CanvasPanel_SkillWheel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> VerticalBox_SkillList;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Slot01;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Slot02;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Close;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Slot01;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Slot02;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_SelectedSkill;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_SelectedSkillName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_SelectedSkillDescription;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_SelectedSkillCooldown;

	UFUNCTION(BlueprintImplementableEvent, Category = "Skill Selection", meta = (DisplayName = "On Skill Selection Refreshed"))
	void BP_OnSkillSelectionRefreshed(
		const TArray<UGP_SkillData*>& AvailableSkills,
		UGP_SkillData* Slot01Skill,
		UGP_SkillData* Slot02Skill,
		FGameplayTag ActiveSlotTag);

private:
	UFUNCTION()
	void HandleEquippedSkillChanged(FGameplayTag SlotTag, UGP_SkillData* SkillData);

	UFUNCTION()
	void HandleSkillEntryClicked(UGP_SkillData* SkillData);

	UFUNCTION()
	void HandleSlot01Clicked();

	UFUNCTION()
	void HandleSlot02Clicked();

	UFUNCTION()
	void HandleCloseClicked();

	AGP_PlayerController* GetGPPlayerController() const;
	AGP_PlayerState* GetGPPlayerState() const;
	void EnsureRuntimeFallbackLayout();
	void SetSelectedSlot(FGameplayTag SlotTag);
	void SetSelectedSkill(UGP_SkillData* SkillData);
	void RebuildSkillEntries(const TArray<UGP_SkillData*>& AvailableSkills, UGP_SkillData* Slot01Skill, UGP_SkillData* Slot02Skill);
	void RefreshSkillEntryStates(UGP_SkillData* Slot01Skill, UGP_SkillData* Slot02Skill);
	void RefreshSelectedSkillDetails() const;
	void RefreshSlotTexts(UGP_SkillData* Slot01Skill, UGP_SkillData* Slot02Skill) const;
	FText GetSlotDisplayText(const TCHAR* SlotLabel, const UGP_SkillData* SkillData) const;
	FText GetCooldownDisplayText(const UGP_SkillData* SkillData) const;

	static constexpr int32 SkillWheelSlotCount = 8;

	UPROPERTY(Transient)
	FGameplayTag SelectedSlotTag;

	UPROPERTY(Transient)
	TObjectPtr<UGP_SkillData> SelectedSkillData;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UGP_SkillEntryWidget>> SkillEntryWidgets;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RuntimeFallbackRoot;

	bool bOverflowWarningIssued = false;
	bool bMissingEntryClassWarningIssued = false;
};
