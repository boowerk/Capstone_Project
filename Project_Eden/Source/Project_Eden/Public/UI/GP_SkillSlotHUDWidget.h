#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "GP_SkillSlotHUDWidget.generated.h"

class UAbilitySystemComponent;
class UGP_SkillData;
class UImage;
class UProgressBar;
class UTextBlock;

/**
 * Single skill slot displayed on the in-game HUD.
 * Shows the skill icon and a cooldown overlay (progress bar + remaining seconds).
 * Call SetupSlot once after the owning player's ASC is ready; the widget polls
 * the ASC every tick to keep the cooldown display up to date.
 */
UCLASS(Abstract, Blueprintable)
class PROJECT_EDEN_API UGP_SkillSlotHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetupSlot(UAbilitySystemComponent* InASC, UGP_SkillData* InSkillData, const FText& InKeyLabel);
	void ClearSlot();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> ProgressBar_Cooldown;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Key;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Cooldown;

private:
	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;

	UPROPERTY(Transient)
	TObjectPtr<UGP_SkillData> SkillData;

	FGameplayTag CooldownTag;
	float CooldownDuration = 0.f;

	void RefreshIcon();
	void RefreshCooldown();
};
