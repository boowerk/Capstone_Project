#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GP_SkillEntryWidget.generated.h"

class UButton;
class UGP_SkillData;
class UImage;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGP_OnSkillEntryClicked, UGP_SkillData*, SkillData);

UCLASS(Abstract, Blueprintable)
class PROJECT_EDEN_API UGP_SkillEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Skill Selection")
	void SetupSkillEntry(UGP_SkillData* NewSkillData);

	UFUNCTION(BlueprintCallable, Category = "Skill Selection")
	void SetEquipped(bool bNewEquipped);

	UFUNCTION(BlueprintCallable, Category = "Skill Selection")
	void SetSelected(bool bNewSelected);

	UFUNCTION(BlueprintCallable, Category = "Skill Selection")
	void SetCompactWheelMode(bool bNewCompactWheelMode);

	UFUNCTION(BlueprintPure, Category = "Skill Selection")
	UGP_SkillData* GetSkillData() const { return SkillData.Get(); }

	UPROPERTY(BlueprintAssignable, Category = "Skill Selection")
	FGP_OnSkillEntryClicked OnSkillEntryClicked;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, Category = "Skill Selection")
	TObjectPtr<UGP_SkillData> SkillData;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Root;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_Selected;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Name;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Description;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Equipped;

private:
	UFUNCTION()
	void HandleRootClicked();

	void EnsureCompactWheelLayout();
	void RefreshView();
	void RefreshStateVisuals();

	bool bIsEquipped = false;
	bool bIsSelected = false;
	bool bCompactWheelMode = false;
	bool bCompactWheelLayoutBuilt = false;
};
