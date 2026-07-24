#include "UI/GP_RunResultWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"

#define LOCTEXT_NAMESPACE "GP_RunResultWidget"

namespace
{
void ConfigureText(
	UTextBlock* TextBlock,
	int32 FontSize,
	const FName& Typeface,
	const FLinearColor& Color,
	ETextJustify::Type Justification = ETextJustify::Center)
{
	if (!IsValid(TextBlock))
	{
		return;
	}

	TextBlock->SetFont(FCoreStyle::GetDefaultFontStyle(Typeface, FontSize));
	TextBlock->SetColorAndOpacity(FSlateColor(Color));
	TextBlock->SetJustification(Justification);
	TextBlock->SetAutoWrapText(true);
	TextBlock->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
	TextBlock->SetShadowOffset(FVector2D(1.5f, 2.0f));
}

void AddVerticalWidget(
	UVerticalBox* VerticalBox,
	UWidget* Widget,
	const FMargin& Padding = FMargin(0.0f))
{
	if (!IsValid(VerticalBox) || !IsValid(Widget))
	{
		return;
	}

	if (UVerticalBoxSlot* Slot = VerticalBox->AddChildToVerticalBox(Widget))
	{
		Slot->SetHorizontalAlignment(HAlign_Fill);
		Slot->SetVerticalAlignment(VAlign_Center);
		Slot->SetPadding(Padding);
	}
}
}

void UGP_RunResultWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BuildNativeWidgetTree();
	SetIsFocusable(false);
	SetVisibility(ESlateVisibility::Collapsed);
	SetRenderOpacity(0.0f);
}

void UGP_RunResultWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (Presentation == EGPRunResultPresentation::Hidden)
	{
		return;
	}

	if (RevealDelayRemaining > 0.0f)
	{
		RevealDelayRemaining = FMath::Max(0.0f, RevealDelayRemaining - InDeltaTime);
		SetRenderOpacity(0.0f);
		return;
	}

	FadeElapsed += InDeltaTime;
	SetRenderOpacity(FMath::Clamp(FadeElapsed / FMath::Max(FadeInDuration, KINDA_SMALL_NUMBER), 0.0f, 1.0f));
}

void UGP_RunResultWidget::ShowEliminated(
	const FText& SpectatedPlayerName,
	int32 LivingPlayerCount,
	float RecoverySecondsRemaining)
{
	const FText Description = LivingPlayerCount > 0
		? FText::Format(
			LOCTEXT("EliminatedDescription", "아군이 전투를 이어가고 있습니다  ·  생존 {0}명"),
			FText::AsNumber(LivingPlayerCount))
		: LOCTEXT("EliminatedDescriptionPending", "파티의 생존 상태를 확인하고 있습니다.");

	const int32 RecoverySeconds = FMath::Max(0, FMath::CeilToInt(RecoverySecondsRemaining));
	FText Status;
	if (RecoverySecondsRemaining <= KINDA_SMALL_NUMBER)
	{
		Status = SpectatedPlayerName.IsEmpty()
			? LOCTEXT("SecuringRecoveryPoint", "안전한 전투 복귀 지점을 확보하는 중…")
			: FText::Format(
				LOCTEXT("SpectatingWhileSecuringRecoveryPoint", "{0} 관전 중  ·  안전한 복귀 지점 확보 중…"),
				SpectatedPlayerName);
	}
	else
	{
		Status = SpectatedPlayerName.IsEmpty()
			? FText::Format(
				LOCTEXT("FindingSpectatorTarget", "생존한 아군을 찾는 중…  ·  {0}초 후 전투 복귀"),
				FText::AsNumber(RecoverySeconds))
			: FText::Format(
				LOCTEXT("SpectatingPlayer", "{0} 관전 중  ·  {1}초 후 전투 복귀"),
				SpectatedPlayerName,
				FText::AsNumber(RecoverySeconds));
	}

	ApplyPresentation(
		EGPRunResultPresentation::Eliminated,
		LOCTEXT("EliminatedEyebrow", "EXPEDITION STATUS"),
		LOCTEXT("EliminatedTitle", "전투 불능"),
		Description,
		Status,
		FLinearColor(0.72f, 0.20f, 0.13f, 1.0f),
		FLinearColor(0.025f, 0.003f, 0.003f, 0.72f),
		0.0f);
}

void UGP_RunResultWidget::ShowVictory()
{
	ApplyPresentation(
		EGPRunResultPresentation::Victory,
		LOCTEXT("VictoryEyebrow", "VICTORY"),
		LOCTEXT("VictoryTitle", "콜로세움 정복"),
		LOCTEXT("VictoryDescription", "콜로세움의 수호자를 쓰러뜨리고 에덴의 숨결을 되찾았습니다."),
		LOCTEXT("VictoryStatus", "잠시 후 파티와 함께 로비로 이동합니다."),
		FLinearColor(0.86f, 0.66f, 0.25f, 1.0f),
		FLinearColor(0.0f, 0.018f, 0.012f, 0.68f),
		VictoryRevealDelay);
}

void UGP_RunResultWidget::ShowDefeat()
{
	ApplyPresentation(
		EGPRunResultPresentation::Defeat,
		LOCTEXT("DefeatEyebrow", "EXPEDITION FAILED"),
		LOCTEXT("DefeatTitle", "원정 실패"),
		LOCTEXT("DefeatDescription", "파티가 전멸하여 더 이상 원정을 이어갈 수 없습니다."),
		LOCTEXT("DefeatStatus", "잠시 후 파티와 함께 로비로 이동합니다."),
		FLinearColor(0.62f, 0.10f, 0.08f, 1.0f),
		FLinearColor(0.02f, 0.0f, 0.0f, 0.82f),
		0.0f);
}

void UGP_RunResultWidget::HidePresentation()
{
	Presentation = EGPRunResultPresentation::Hidden;
	RevealDelayRemaining = 0.0f;
	FadeElapsed = 0.0f;
	SetRenderOpacity(0.0f);
	SetVisibility(ESlateVisibility::Collapsed);
}

void UGP_RunResultWidget::BuildNativeWidgetTree()
{
	if (!IsValid(WidgetTree) || IsValid(WidgetTree->RootWidget))
	{
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	ScreenTint = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ScreenTint"));
	ScreenTint->SetPadding(FMargin(0.0f));
	if (UCanvasPanelSlot* TintSlot = RootCanvas->AddChildToCanvas(ScreenTint))
	{
		TintSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		TintSlot->SetOffsets(FMargin(0.0f));
	}

	USizeBox* CardSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ResultCardSize"));
	CardSize->SetWidthOverride(820.0f);
	CardSize->SetMinDesiredHeight(320.0f);
	if (UCanvasPanelSlot* CardSlot = RootCanvas->AddChildToCanvas(CardSize))
	{
		CardSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		CardSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CardSlot->SetAutoSize(true);
	}

	ResultCard = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ResultCard"));
	ResultCard->SetPadding(FMargin(54.0f, 38.0f, 54.0f, 42.0f));
	ResultCard->SetBrushColor(FLinearColor(0.008f, 0.009f, 0.012f, 0.94f));
	CardSize->SetContent(ResultCard);

	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ResultContent"));
	ResultCard->SetContent(Content);

	EyebrowText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EyebrowText"));
	ConfigureText(EyebrowText, 18, TEXT("Bold"), FLinearColor::White);
	FSlateFontInfo EyebrowFont = EyebrowText->GetFont();
	EyebrowFont.LetterSpacing = 240;
	EyebrowText->SetFont(EyebrowFont);
	AddVerticalWidget(Content, EyebrowText);

	USpacer* TitleSpacing = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("TitleSpacing"));
	TitleSpacing->SetSize(FVector2D(1.0f, 10.0f));
	AddVerticalWidget(Content, TitleSpacing);

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	ConfigureText(TitleText, 56, TEXT("Bold"), FLinearColor::White);
	AddVerticalWidget(Content, TitleText);

	AccentLine = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("AccentLine"));
	AccentLine->SetPadding(FMargin(0.0f, 1.0f));
	AccentLine->SetDesiredSizeScale(FVector2D(1.0f, 1.0f));
	AddVerticalWidget(Content, AccentLine, FMargin(0.0f, 20.0f, 0.0f, 20.0f));

	DescriptionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DescriptionText"));
	ConfigureText(DescriptionText, 21, TEXT("Regular"), FLinearColor(0.88f, 0.88f, 0.86f, 1.0f));
	AddVerticalWidget(Content, DescriptionText);

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
	ConfigureText(StatusText, 17, TEXT("Regular"), FLinearColor(0.68f, 0.70f, 0.72f, 1.0f));
	AddVerticalWidget(Content, StatusText, FMargin(0.0f, 20.0f, 0.0f, 0.0f));
}

void UGP_RunResultWidget::ApplyPresentation(
	EGPRunResultPresentation NewPresentation,
	const FText& Eyebrow,
	const FText& Title,
	const FText& Description,
	const FText& Status,
	const FLinearColor& AccentColor,
	const FLinearColor& ScreenTintColor,
	float RevealDelay)
{
	const bool bPresentationChanged = Presentation != NewPresentation;
	Presentation = NewPresentation;

	if (IsValid(ScreenTint))
	{
		ScreenTint->SetBrushColor(ScreenTintColor);
	}
	if (IsValid(ResultCard))
	{
		ResultCard->SetBrushColor(FLinearColor(0.008f, 0.009f, 0.012f, 0.94f));
	}
	if (IsValid(AccentLine))
	{
		AccentLine->SetBrushColor(AccentColor);
	}
	if (IsValid(EyebrowText))
	{
		EyebrowText->SetText(Eyebrow);
		EyebrowText->SetColorAndOpacity(FSlateColor(AccentColor));
	}
	if (IsValid(TitleText))
	{
		TitleText->SetText(Title);
	}
	if (IsValid(DescriptionText))
	{
		DescriptionText->SetText(Description);
	}
	if (IsValid(StatusText))
	{
		StatusText->SetText(Status);
	}

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (bPresentationChanged)
	{
		RevealDelayRemaining = FMath::Max(0.0f, RevealDelay);
		FadeElapsed = 0.0f;
		SetRenderOpacity(0.0f);
	}
}

#undef LOCTEXT_NAMESPACE
