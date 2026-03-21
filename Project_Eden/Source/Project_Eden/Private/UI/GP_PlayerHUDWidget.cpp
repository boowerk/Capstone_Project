#include "UI/GP_PlayerHUDWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

UGP_PlayerHUDWidget::UGP_PlayerHUDWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	LocationText = FText::FromString(TEXT("LIMINAL ASHEN FIELD"));
	BossText = FText::FromString(TEXT("Omen of the Drowned Belfry"));
}

void UGP_PlayerHUDWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	RefreshPreview();
}

void UGP_PlayerHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshPreview();
}

void UGP_PlayerHUDWidget::SetVitals(float InHealthPercent, float InFocusPercent, float InStaminaPercent)
{
	HealthPercent = FMath::Clamp(InHealthPercent, 0.0f, 1.0f);
	FocusPercent = FMath::Clamp(InFocusPercent, 0.0f, 1.0f);
	StaminaPercent = FMath::Clamp(InStaminaPercent, 0.0f, 1.0f);
	RefreshPreview();
}

void UGP_PlayerHUDWidget::SetLocationText(const FText& InLocationText)
{
	LocationText = InLocationText;
	RefreshPreview();
}

void UGP_PlayerHUDWidget::SetBossText(const FText& InBossText)
{
	BossText = InBossText;
	RefreshPreview();
}

void UGP_PlayerHUDWidget::SetBossVisible(bool bIsVisible)
{
	bShowBossFrame = bIsVisible;
	RefreshPreview();
}

void UGP_PlayerHUDWidget::RefreshPreview()
{
	if (HealthBar)
	{
		HealthBar->SetPercent(HealthPercent);
	}

	if (FocusBar)
	{
		FocusBar->SetPercent(FocusPercent);
	}

	if (StaminaBar)
	{
		StaminaBar->SetPercent(StaminaPercent);
	}

	if (LocationTextBlock)
	{
		LocationTextBlock->SetText(LocationText);
	}

	if (BossTextBlock)
	{
		BossTextBlock->SetText(BossText);
	}

	if (BossFrame)
	{
		BossFrame->SetVisibility(bShowBossFrame ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}
