#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GP_AugmentSelectWidget.generated.h"

class AGP_PlayerState;
class UButton;
class UGP_SkillAugmentData;
class UTextBlock;

UCLASS()
class PROJECT_EDEN_API UGP_AugmentSelectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
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

	UFUNCTION()
	void SelectAugment0();

	UFUNCTION()
	void SelectAugment1();

	UFUNCTION()
	void SelectAugment2();

	void RefreshCandidateTexts();
	FText GetAugmentDisplayText(const UGP_SkillAugmentData* AugmentData) const;
	AGP_PlayerState* GetGPPlayerState() const;
};
