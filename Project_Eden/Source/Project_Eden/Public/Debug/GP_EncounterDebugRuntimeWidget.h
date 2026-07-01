#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
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
	TArray<TWeakObjectPtr<AGP_EnemyCharacter>> EnemyOptions;

	void BuildPanel();
	UButton* AddButton(UVerticalBox* Parent, const FText& Label);
	UButton* AddRowButton(class UHorizontalBox* Row, const FText& Label);
	AGP_EncounterDebugDirector* GetDirector() const;
	void RefreshMobClassOptions();
	void RefreshEnemyOptions();

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
