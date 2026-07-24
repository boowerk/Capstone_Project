#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GP_RunResultWidget.generated.h"

class UBorder;
class UTextBlock;

UENUM(BlueprintType)
enum class EGPRunResultPresentation : uint8
{
	Hidden,
	Eliminated,
	Victory,
	Defeat
};

/**
 * Full-screen run-state presentation used for local elimination, final victory,
 * and party defeat. The native widget tree is a cooked fallback, while an
 * optional Widget Blueprint child can replace the visuals by binding widgets
 * with the same names.
 */
UCLASS()
class PROJECT_EDEN_API UGP_RunResultWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UI|Run Result")
	void ShowEliminated(
		const FText& SpectatedPlayerName,
		int32 LivingPlayerCount,
		float RecoverySecondsRemaining);

	UFUNCTION(BlueprintCallable, Category = "UI|Run Result")
	void ShowVictory();

	UFUNCTION(BlueprintCallable, Category = "UI|Run Result")
	void ShowDefeat();

	UFUNCTION(BlueprintCallable, Category = "UI|Run Result")
	void HidePresentation();

	UFUNCTION(BlueprintPure, Category = "UI|Run Result")
	EGPRunResultPresentation GetPresentation() const { return Presentation; }

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// Leaves the boss death presentation readable before the final victory card fades in.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Run Result", meta = (ClampMin = "0.0", Units = "s"))
	float VictoryRevealDelay = 1.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Run Result", meta = (ClampMin = "0.01", Units = "s"))
	float FadeInDuration = 0.35f;

private:
	void BuildNativeWidgetTree();
	void ApplyPresentation(
		EGPRunResultPresentation NewPresentation,
		const FText& Eyebrow,
		const FText& Title,
		const FText& Description,
		const FText& Status,
		const FLinearColor& AccentColor,
		const FLinearColor& ScreenTintColor,
		float RevealDelay);

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UBorder> ScreenTint;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UBorder> ResultCard;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UBorder> AccentLine;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EyebrowText;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DescriptionText;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

	EGPRunResultPresentation Presentation = EGPRunResultPresentation::Hidden;
	float RevealDelayRemaining = 0.0f;
	float FadeElapsed = 0.0f;
};
