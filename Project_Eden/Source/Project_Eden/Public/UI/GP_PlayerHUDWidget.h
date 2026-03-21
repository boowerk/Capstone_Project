#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GP_PlayerHUDWidget.generated.h"

class UTextBlock;
class UProgressBar;
class UWidget;

UCLASS()
class PROJECT_EDEN_API UGP_PlayerHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UGP_PlayerHUDWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "EldenRing HUD")
	void SetVitals(float InHealthPercent, float InFocusPercent, float InStaminaPercent);

	UFUNCTION(BlueprintCallable, Category = "EldenRing HUD")
	void SetLocationText(const FText& InLocationText);

	UFUNCTION(BlueprintCallable, Category = "EldenRing HUD")
	void SetBossText(const FText& InBossText);

	UFUNCTION(BlueprintCallable, Category = "EldenRing HUD")
	void SetBossVisible(bool bIsVisible);

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

private:
	void RefreshPreview();

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UProgressBar> HealthBar;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UProgressBar> FocusBar;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UProgressBar> StaminaBar;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> LocationTextBlock;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> BossTextBlock;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWidget> BossFrame;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EldenRing HUD|Preview", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0"))
	float HealthPercent = 0.82f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EldenRing HUD|Preview", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0"))
	float FocusPercent = 0.58f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EldenRing HUD|Preview", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0"))
	float StaminaPercent = 0.91f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EldenRing HUD|Preview", meta = (AllowPrivateAccess = "true"))
	FText LocationText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EldenRing HUD|Preview", meta = (AllowPrivateAccess = "true"))
	FText BossText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EldenRing HUD|Preview", meta = (AllowPrivateAccess = "true"))
	bool bShowBossFrame = false;
};
