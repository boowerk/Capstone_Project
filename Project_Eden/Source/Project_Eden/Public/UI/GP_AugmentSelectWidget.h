#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_SkillAugmentData.h"
#include "Blueprint/UserWidget.h"
#include "GP_AugmentSelectWidget.generated.h"

class AGP_PlayerState;
class UButton;
class UImage;
class UTextBlock;
class UTexture2D;

UCLASS()
class PROJECT_EDEN_API UGP_AugmentSelectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UGP_AugmentSelectWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Tech|Augment")
	void SetCandidateAugments(const TArray<UGP_SkillAugmentData*>& NewCandidateAugments);

	UFUNCTION(BlueprintCallable, Category = "Tech|Augment")
	bool SelectCandidateIndex(int32 CandidateIndex);

	UFUNCTION(BlueprintPure, Category = "Tech|Augment")
	UGP_SkillAugmentData* GetCandidateAugment(int32 CandidateIndex) const;

protected:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Tech|Augment")
	void OnAugmentSelected(UGP_SkillAugmentData* AugmentData);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tech|Augment")
	bool bRemoveOnSelection = true;

	UPROPERTY(BlueprintReadOnly, Category = "Tech|Augment")
	TArray<TObjectPtr<UGP_SkillAugmentData>> CandidateAugments;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tech|Augment|Visual")
	TMap<EGP_SkillAugmentType, TSoftObjectPtr<UTexture2D>> AugmentTypeBackgrounds;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Augment0;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Augment1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Augment2;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextBlock_Augment0;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextBlock_Augment1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextBlock_Augment2;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextBlock_Description0;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextBlock_Description1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextBlock_Description2;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_Icon0;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_Icon1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_Icon2;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_CardBg0;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_CardBg1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_CardBg2;

	UFUNCTION()
	void SelectAugment0();

	UFUNCTION()
	void SelectAugment1();

	UFUNCTION()
	void SelectAugment2();

	void RefreshCandidateViews();
	void RefreshCandidateView(int32 CandidateIndex, UTextBlock* NameText, UTextBlock* DescriptionText, UImage* IconImage, UImage* BackgroundImage);
	FText GetAugmentDisplayText(const UGP_SkillAugmentData* AugmentData) const;
	UTexture2D* GetAugmentTypeBackgroundTexture(const UGP_SkillAugmentData* AugmentData) const;
	AGP_PlayerState* GetGPPlayerState() const;
};
