#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Debug/GP_EncounterDebugDirector.h"
#include "Types/SlateEnums.h"
#include "Widgets/SWidget.h"
#include "GP_EncounterDebugRuntimeWidget.generated.h"

class AGP_EncounterDebugDirector;
class AGP_EnemyCharacter;
class UButton;
class UComboBoxString;
class UTextBlock;
class UVerticalBox;

UCLASS()
class PROJECT_EDEN_API UGP_EncounterDebugRuntimeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetDirector(AGP_EncounterDebugDirector* InDirector);
	void SetStatusMessage(const FString& Message);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<AGP_EncounterDebugDirector> Director;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(Transient)
	TObjectPtr<UComboBoxString> MobClassCombo;

	UPROPERTY(Transient)
	TObjectPtr<UComboBoxString> EnemyCombo;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SelectedEnemyHeaderText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SelectedEnemySkillText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SelectedEnemyLinkedText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SelectedEnemyDetailTitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SelectedEnemyInfoText;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AGP_EnemyCharacter>> EnemyOptions;

	EGPEncounterDebugReportMode ReportMode = EGPEncounterDebugReportMode::Summary;

	float EnemyInfoRefreshElapsed = 0.0f;

	void BuildPanel();
	UButton* AddButton(UVerticalBox* Parent, const FText& Label);
	UButton* AddRowButton(class UHorizontalBox* Row, const FText& Label);
	UTextBlock* AddMonitorCard(UVerticalBox* Parent, const FText& Title, const FLinearColor& AccentColor, float HeightOverride = 0.0f);
	AGP_EncounterDebugDirector* GetDirector() const;
	void RefreshMobClassOptions();
	void RefreshEnemyOptions();
	void RefreshSelectedEnemyInfo();

	UFUNCTION()
	void HandleMobClassSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleEnemySelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleValidateClicked();

	UFUNCTION()
	void HandleStage1Clicked();

	UFUNCTION()
	void HandleStage2Clicked();

	UFUNCTION()
	void HandleStage3Clicked();

	UFUNCTION()
	void HandleTeleportClicked();

	UFUNCTION()
	void HandleSpawnMobClicked();

	UFUNCTION()
	void HandleSpawnBossClicked();

	UFUNCTION()
	void HandleSpawnSelectedClicked();

	UFUNCTION()
	void HandleRefreshClicked();

	UFUNCTION()
	void HandleReportSummaryClicked();

	UFUNCTION()
	void HandleReportAIClicked();

	UFUNCTION()
	void HandleReportSkillClicked();

	UFUNCTION()
	void HandleReportGASClicked();

	UFUNCTION()
	void HandleReportMatadorClicked();

	UFUNCTION()
	void HandleReportAllClicked();

	UFUNCTION()
	void HandleClearClicked();

	UFUNCTION()
	void HandleKillClicked();

	UFUNCTION()
	void HandleDestroyClicked();

	UFUNCTION()
	void HandleHp100Clicked();

	UFUNCTION()
	void HandleHp50Clicked();

	UFUNCTION()
	void HandleHp1Clicked();

	UFUNCTION()
	void HandleAIOnClicked();

	UFUNCTION()
	void HandleAIOffClicked();

	UFUNCTION()
	void HandleCloseClicked();
};
