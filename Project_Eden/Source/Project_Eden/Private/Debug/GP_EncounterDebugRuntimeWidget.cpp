#include "Debug/GP_EncounterDebugRuntimeWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ComboBoxString.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Characters/GP_EnemyCharacter.h"
#include "Debug/GP_EncounterDebugDirector.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateColor.h"

void UGP_EncounterDebugRuntimeWidget::SetDirector(AGP_EncounterDebugDirector* InDirector)
{
	Director = InDirector;
	RefreshMobClassOptions();
}

void UGP_EncounterDebugRuntimeWidget::SetStatusMessage(const FString& Message)
{
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(Message));
	}
}

TSharedRef<SWidget> UGP_EncounterDebugRuntimeWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildPanel();
	}

	return Super::RebuildWidget();
}

void UGP_EncounterDebugRuntimeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	BuildPanel();
}

void UGP_EncounterDebugRuntimeWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	EnemyInfoRefreshElapsed += InDeltaTime;
	if (EnemyInfoRefreshElapsed >= 0.1f)
	{
		EnemyInfoRefreshElapsed = 0.0f;
		RefreshSelectedEnemyInfo();
	}
}

void UGP_EncounterDebugRuntimeWidget::BuildPanel()
{
	UCanvasPanel* ScreenRoot = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("EncounterDebugScreenRoot"));
	WidgetTree->RootWidget = ScreenRoot;

	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("EncounterDebugPanelSize"));
	PanelSize->SetWidthOverride(540.0f);
	if (UCanvasPanelSlot* PanelSlot = ScreenRoot->AddChildToCanvas(PanelSize))
	{
		PanelSlot->SetAnchors(FAnchors(1.0f, 0.0f, 1.0f, 0.0f));
		PanelSlot->SetAlignment(FVector2D(1.0f, 0.0f));
		PanelSlot->SetPosition(FVector2D(-24.0f, 118.0f));
		PanelSlot->SetAutoSize(true);
	}

	UBorder* PanelBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("EncounterDebugPanelBackground"));
	PanelBackground->SetBrushColor(FLinearColor(0.02f, 0.025f, 0.03f, 0.72f));
	PanelBackground->SetPadding(FMargin(8.0f));
	PanelSize->AddChild(PanelBackground);

	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("EncounterDebugRoot"));
	PanelBackground->SetContent(Root);

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EncounterDebugStatus"));
	StatusText->SetText(FText::FromString(TEXT("Encounter Debug")));
	StatusText->SetAutoWrapText(true);
	StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.88f, 0.96f, 1.0f, 1.0f)));
	StatusText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 13));
	if (UVerticalBoxSlot* StatusSlot = Root->AddChildToVerticalBox(StatusText))
	{
		StatusSlot->SetPadding(FMargin(4.0f, 2.0f, 4.0f, 6.0f));
	}

	UButton* ValidateButton = AddButton(Root, FText::FromString(TEXT("Validate Setup")));
	ValidateButton->OnClicked.AddDynamic(this, &UGP_EncounterDebugRuntimeWidget::HandleValidateClicked);

	MobClassCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("EnemyClassCombo"));
	MobClassCombo->OnSelectionChanged.AddDynamic(this, &UGP_EncounterDebugRuntimeWidget::HandleMobClassSelectionChanged);
	if (UVerticalBoxSlot* ComboSlot = Root->AddChildToVerticalBox(MobClassCombo))
	{
		ComboSlot->SetPadding(FMargin(2.0f, 2.0f, 2.0f, 6.0f));
	}
	RefreshMobClassOptions();

	UButton* SpawnSelectedButton = AddButton(Root, FText::FromString(TEXT("Spawn Selected")));
	SpawnSelectedButton->OnClicked.AddDynamic(this, &UGP_EncounterDebugRuntimeWidget::HandleSpawnSelectedClicked);

	EnemyCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("EnemyCombo"));
	EnemyCombo->OnSelectionChanged.AddDynamic(this, &UGP_EncounterDebugRuntimeWidget::HandleEnemySelectionChanged);
	if (UVerticalBoxSlot* ComboSlot = Root->AddChildToVerticalBox(EnemyCombo))
	{
		ComboSlot->SetPadding(FMargin(2.0f, 2.0f, 2.0f, 6.0f));
	}
	RefreshEnemyOptions();

	UHorizontalBox* ReportModeRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ReportModeRow"));
	if (UVerticalBoxSlot* RowSlot = Root->AddChildToVerticalBox(ReportModeRow))
	{
		RowSlot->SetPadding(FMargin(4.0f, 0.0f, 4.0f, 4.0f));
	}
	UButton* SummaryButton = AddRowButton(ReportModeRow, FText::FromString(TEXT("Sum")));
	SummaryButton->OnClicked.AddDynamic(this, &UGP_EncounterDebugRuntimeWidget::HandleReportSummaryClicked);
	UButton* AIButton = AddRowButton(ReportModeRow, FText::FromString(TEXT("AI")));
	AIButton->OnClicked.AddDynamic(this, &UGP_EncounterDebugRuntimeWidget::HandleReportAIClicked);
	UButton* SkillButton = AddRowButton(ReportModeRow, FText::FromString(TEXT("Skill")));
	SkillButton->OnClicked.AddDynamic(this, &UGP_EncounterDebugRuntimeWidget::HandleReportSkillClicked);
	UButton* GASButton = AddRowButton(ReportModeRow, FText::FromString(TEXT("GAS")));
	GASButton->OnClicked.AddDynamic(this, &UGP_EncounterDebugRuntimeWidget::HandleReportGASClicked);
	UButton* MatadorButton = AddRowButton(ReportModeRow, FText::FromString(TEXT("Mat")));
	MatadorButton->OnClicked.AddDynamic(this, &UGP_EncounterDebugRuntimeWidget::HandleReportMatadorClicked);
	UButton* AllButton = AddRowButton(ReportModeRow, FText::FromString(TEXT("All")));
	AllButton->OnClicked.AddDynamic(this, &UGP_EncounterDebugRuntimeWidget::HandleReportAllClicked);

	SelectedEnemyHeaderText = AddMonitorCard(Root, FText::FromString(TEXT("Selected")), FLinearColor(0.20f, 0.52f, 0.92f, 1.0f), 74.0f);

	SelectedEnemySkillText = AddMonitorCard(Root, FText::FromString(TEXT("Next Skill")), FLinearColor(0.95f, 0.68f, 0.24f, 1.0f), 82.0f);

	SelectedEnemyLinkedText = AddMonitorCard(Root, FText::FromString(TEXT("Linked Actors")), FLinearColor(0.42f, 0.86f, 0.62f, 1.0f), 98.0f);

	SelectedEnemyDetailTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SelectedEnemyDetailTitleText"));
	SelectedEnemyDetailTitleText->SetText(FText::FromString(TEXT("Detail")));
	SelectedEnemyDetailTitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.82f, 0.88f, 0.94f, 1.0f)));
	SelectedEnemyDetailTitleText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 10));
	if (UVerticalBoxSlot* DetailTitleSlot = Root->AddChildToVerticalBox(SelectedEnemyDetailTitleText))
	{
		DetailTitleSlot->SetPadding(FMargin(4.0f, 2.0f, 4.0f, 1.0f));
	}

	USizeBox* InfoSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SelectedEnemyInfoSize"));
	InfoSize->SetHeightOverride(120.0f);
	if (UVerticalBoxSlot* InfoSizeSlot = Root->AddChildToVerticalBox(InfoSize))
	{
		InfoSizeSlot->SetPadding(FMargin(2.0f, 0.0f, 2.0f, 6.0f));
	}

	UScrollBox* InfoScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("SelectedEnemyInfoScroll"));
	InfoSize->AddChild(InfoScroll);

	SelectedEnemyInfoText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SelectedEnemyInfoText"));
	SelectedEnemyInfoText->SetText(FText::FromString(TEXT("<none>")));
	SelectedEnemyInfoText->SetAutoWrapText(true);
	SelectedEnemyInfoText->SetColorAndOpacity(FSlateColor(FLinearColor(0.78f, 0.88f, 0.92f, 1.0f)));
	SelectedEnemyInfoText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 9));
	InfoScroll->AddChild(SelectedEnemyInfoText);
	RefreshSelectedEnemyInfo();

	UHorizontalBox* StageRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("StageRow"));
	if (UVerticalBoxSlot* RowSlot = Root->AddChildToVerticalBox(StageRow))
	{
		RowSlot->SetPadding(FMargin(4.0f));
	}
	UButton* Stage1Button = AddRowButton(StageRow, FText::FromString(TEXT("Stage 1")));
	Stage1Button->OnClicked.AddDynamic(this, &UGP_EncounterDebugRuntimeWidget::HandleStage1Clicked);
	UButton* Stage2Button = AddRowButton(StageRow, FText::FromString(TEXT("Stage 2")));
	Stage2Button->OnClicked.AddDynamic(this, &UGP_EncounterDebugRuntimeWidget::HandleStage2Clicked);
	UButton* Stage3Button = AddRowButton(StageRow, FText::FromString(TEXT("Stage 3")));
	Stage3Button->OnClicked.AddDynamic(this, &UGP_EncounterDebugRuntimeWidget::HandleStage3Clicked);

	UButton* TeleportButton = AddButton(Root, FText::FromString(TEXT("Teleport Player")));
	TeleportButton->OnClicked.AddDynamic(this, &UGP_EncounterDebugRuntimeWidget::HandleTeleportClicked);

	UHorizontalBox* StageSpawnRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("StageSpawnRow"));
	if (UVerticalBoxSlot* RowSlot = Root->AddChildToVerticalBox(StageSpawnRow))
	{
		RowSlot->SetPadding(FMargin(4.0f));
	}
	UButton* SpawnMobButton = AddRowButton(StageSpawnRow, FText::FromString(TEXT("Spawn Stage Mob")));
	SpawnMobButton->OnClicked.AddDynamic(this, &UGP_EncounterDebugRuntimeWidget::HandleSpawnMobClicked);
	UButton* SpawnBossButton = AddRowButton(StageSpawnRow, FText::FromString(TEXT("Spawn Stage Boss")));
	SpawnBossButton->OnClicked.AddDynamic(this, &UGP_EncounterDebugRuntimeWidget::HandleSpawnBossClicked);

	UHorizontalBox* EnemyRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("EnemyRow"));
	if (UVerticalBoxSlot* RowSlot = Root->AddChildToVerticalBox(EnemyRow))
	{
		RowSlot->SetPadding(FMargin(4.0f));
	}
	UButton* RefreshButton = AddRowButton(EnemyRow, FText::FromString(TEXT("Refresh")));
	RefreshButton->OnClicked.AddDynamic(this, &UGP_EncounterDebugRuntimeWidget::HandleRefreshClicked);
	UButton* ClearButton = AddRowButton(EnemyRow, FText::FromString(TEXT("Clear")));
	ClearButton->OnClicked.AddDynamic(this, &UGP_EncounterDebugRuntimeWidget::HandleClearClicked);
	UButton* KillButton = AddRowButton(EnemyRow, FText::FromString(TEXT("Kill")));
	KillButton->OnClicked.AddDynamic(this, &UGP_EncounterDebugRuntimeWidget::HandleKillClicked);
	UButton* DestroyButton = AddRowButton(EnemyRow, FText::FromString(TEXT("Destroy")));
	DestroyButton->OnClicked.AddDynamic(this, &UGP_EncounterDebugRuntimeWidget::HandleDestroyClicked);

	UHorizontalBox* HpRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HpRow"));
	if (UVerticalBoxSlot* RowSlot = Root->AddChildToVerticalBox(HpRow))
	{
		RowSlot->SetPadding(FMargin(4.0f));
	}
	UButton* Hp100Button = AddRowButton(HpRow, FText::FromString(TEXT("HP 100")));
	Hp100Button->OnClicked.AddDynamic(this, &UGP_EncounterDebugRuntimeWidget::HandleHp100Clicked);
	UButton* Hp50Button = AddRowButton(HpRow, FText::FromString(TEXT("HP 50")));
	Hp50Button->OnClicked.AddDynamic(this, &UGP_EncounterDebugRuntimeWidget::HandleHp50Clicked);
	UButton* Hp1Button = AddRowButton(HpRow, FText::FromString(TEXT("HP 1")));
	Hp1Button->OnClicked.AddDynamic(this, &UGP_EncounterDebugRuntimeWidget::HandleHp1Clicked);

	UHorizontalBox* AiRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("AIRow"));
	if (UVerticalBoxSlot* RowSlot = Root->AddChildToVerticalBox(AiRow))
	{
		RowSlot->SetPadding(FMargin(4.0f));
	}
	UButton* AIOnButton = AddRowButton(AiRow, FText::FromString(TEXT("AI On")));
	AIOnButton->OnClicked.AddDynamic(this, &UGP_EncounterDebugRuntimeWidget::HandleAIOnClicked);
	UButton* AIOffButton = AddRowButton(AiRow, FText::FromString(TEXT("AI Off")));
	AIOffButton->OnClicked.AddDynamic(this, &UGP_EncounterDebugRuntimeWidget::HandleAIOffClicked);

	UButton* CloseButton = AddButton(Root, FText::FromString(TEXT("Close")));
	CloseButton->OnClicked.AddDynamic(this, &UGP_EncounterDebugRuntimeWidget::HandleCloseClicked);
}

UButton* UGP_EncounterDebugRuntimeWidget::AddButton(UVerticalBox* Parent, const FText& Label)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Button->SetBackgroundColor(FLinearColor(0.18f, 0.20f, 0.22f, 0.9f));
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Text->SetText(Label);
	Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	Text->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 12));
	Text->SetJustification(ETextJustify::Center);
	Button->AddChild(Text);

	if (UVerticalBoxSlot* ButtonSlot = Parent->AddChildToVerticalBox(Button))
	{
		ButtonSlot->SetPadding(FMargin(2.0f));
	}

	return Button;
}

UButton* UGP_EncounterDebugRuntimeWidget::AddRowButton(UHorizontalBox* Row, const FText& Label)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Button->SetBackgroundColor(FLinearColor(0.18f, 0.20f, 0.22f, 0.9f));
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Text->SetText(Label);
	Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	Text->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 12));
	Text->SetJustification(ETextJustify::Center);
	Button->AddChild(Text);

	if (UHorizontalBoxSlot* ButtonSlot = Row->AddChildToHorizontalBox(Button))
	{
		ButtonSlot->SetPadding(FMargin(2.0f));
		ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	return Button;
}

UTextBlock* UGP_EncounterDebugRuntimeWidget::AddMonitorCard(UVerticalBox* Parent, const FText& Title, const FLinearColor& AccentColor, float HeightOverride)
{
	USizeBox* CardSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	if (HeightOverride > 0.0f)
	{
		CardSize->SetHeightOverride(HeightOverride);
	}
	if (UVerticalBoxSlot* CardSlot = Parent->AddChildToVerticalBox(CardSize))
	{
		CardSlot->SetPadding(FMargin(2.0f, 2.0f, 2.0f, 4.0f));
	}

	UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Card->SetBrushColor(FLinearColor(0.035f, 0.045f, 0.055f, 0.86f));
	Card->SetPadding(FMargin(7.0f, 5.0f));
	CardSize->AddChild(Card);

	UVerticalBox* CardRoot = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Card->SetContent(CardRoot);

	UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TitleText->SetText(Title);
	TitleText->SetColorAndOpacity(FSlateColor(AccentColor));
	TitleText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 9));
	if (UVerticalBoxSlot* TitleSlot = CardRoot->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 3.0f));
	}

	UTextBlock* BodyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	BodyText->SetText(FText::FromString(TEXT("<none>")));
	BodyText->SetAutoWrapText(true);
	BodyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.96f, 0.98f, 1.0f)));
	BodyText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 10));
	if (UVerticalBoxSlot* BodySlot = CardRoot->AddChildToVerticalBox(BodyText))
	{
		BodySlot->SetPadding(FMargin(0.0f));
	}

	return BodyText;
}

AGP_EncounterDebugDirector* UGP_EncounterDebugRuntimeWidget::GetDirector() const
{
	return Director.IsValid() ? Director.Get() : AGP_EncounterDebugDirector::FindActiveEncounterDebugDirector();
}

void UGP_EncounterDebugRuntimeWidget::RefreshMobClassOptions()
{
	if (!MobClassCombo)
	{
		return;
	}

	MobClassCombo->ClearOptions();

	AGP_EncounterDebugDirector* DebugDirector = GetDirector();
	if (!DebugDirector)
	{
		MobClassCombo->AddOption(TEXT("<No Director>"));
		MobClassCombo->SetSelectedIndex(0);
		return;
	}

	const TArray<FString> ClassNames = DebugDirector->GetAvailableMobClassNames();
	if (ClassNames.Num() <= 0)
	{
		MobClassCombo->AddOption(TEXT("<No Class Options>"));
		MobClassCombo->SetSelectedIndex(0);
		return;
	}

	for (const FString& ClassName : ClassNames)
	{
		MobClassCombo->AddOption(ClassName);
	}

	const int32 SelectedIndex = FMath::Clamp(DebugDirector->GetSelectedAvailableMobClassIndex(), 0, ClassNames.Num() - 1);
	MobClassCombo->SetSelectedIndex(SelectedIndex);
}

void UGP_EncounterDebugRuntimeWidget::RefreshEnemyOptions()
{
	if (!EnemyCombo)
	{
		return;
	}

	EnemyCombo->ClearOptions();
	EnemyOptions.Reset();

	AGP_EncounterDebugDirector* DebugDirector = GetDirector();
	if (!DebugDirector)
	{
		EnemyCombo->AddOption(TEXT("<No Director>"));
		EnemyCombo->SetSelectedIndex(0);
		RefreshSelectedEnemyInfo();
		return;
	}

	const TArray<FGPEncounterDebugEnemySnapshot> Snapshots = DebugDirector->RefreshEnemyList();
	if (Snapshots.Num() <= 0)
	{
		EnemyCombo->AddOption(TEXT("<No Enemies>"));
		EnemyCombo->SetSelectedIndex(0);
		RefreshSelectedEnemyInfo();
		return;
	}

	int32 SelectedIndex = INDEX_NONE;
	AGP_EnemyCharacter* SelectedEnemy = DebugDirector->GetSelectedEnemy();
	for (int32 Index = 0; Index < Snapshots.Num(); ++Index)
	{
		const FGPEncounterDebugEnemySnapshot& Snapshot = Snapshots[Index];
		EnemyOptions.Add(Snapshot.Enemy);

		const FString StageText = Snapshot.StageIndex >= 0 ? FString::Printf(TEXT("S%d"), Snapshot.StageIndex + 1) : TEXT("S?");
		const FString HpText = Snapshot.MaxHealth > KINDA_SMALL_NUMBER
			? FString::Printf(TEXT("%.0f%%"), Snapshot.HealthPercent * 100.0f)
			: TEXT("HP?");
		const FString OptionLabel = FString::Printf(
			TEXT("%02d %s %s %s"),
			Index + 1,
			*StageText,
			Snapshot.bBoss ? TEXT("Boss") : TEXT("Mob"),
			*Snapshot.Name);
		EnemyCombo->AddOption(FString::Printf(TEXT("%s [%s]"), *OptionLabel, *HpText));

		if (Snapshot.Enemy == SelectedEnemy)
		{
			SelectedIndex = Index;
		}
	}

	EnemyCombo->SetSelectedIndex(SelectedIndex != INDEX_NONE ? SelectedIndex : 0);
	RefreshSelectedEnemyInfo();
}

void UGP_EncounterDebugRuntimeWidget::RefreshSelectedEnemyInfo()
{
	if (!SelectedEnemyInfoText && !SelectedEnemyHeaderText && !SelectedEnemySkillText && !SelectedEnemyLinkedText)
	{
		return;
	}

	if (AGP_EncounterDebugDirector* DebugDirector = GetDirector())
	{
		if (SelectedEnemyHeaderText)
		{
			SelectedEnemyHeaderText->SetText(FText::FromString(DebugDirector->GetSelectedEnemyHeaderReport()));
		}
		if (SelectedEnemySkillText)
		{
			SelectedEnemySkillText->SetText(FText::FromString(DebugDirector->GetSelectedEnemySkillReport()));
		}
		if (SelectedEnemyLinkedText)
		{
			SelectedEnemyLinkedText->SetText(FText::FromString(DebugDirector->GetSelectedEnemyLinkedActorReport()));
		}
		if (SelectedEnemyDetailTitleText)
		{
			SelectedEnemyDetailTitleText->SetText(FText::FromString(FString::Printf(
				TEXT("Detail: %s"),
				*StaticEnum<EGPEncounterDebugReportMode>()->GetNameStringByValue(static_cast<int64>(ReportMode)))));
		}
		if (SelectedEnemyInfoText)
		{
			SelectedEnemyInfoText->SetText(FText::FromString(DebugDirector->GetSelectedEnemyDebugReport(ReportMode)));
		}
	}
	else
	{
		const FText NoDirectorText = FText::FromString(TEXT("<No Director>"));
		if (SelectedEnemyHeaderText)
		{
			SelectedEnemyHeaderText->SetText(NoDirectorText);
		}
		if (SelectedEnemySkillText)
		{
			SelectedEnemySkillText->SetText(NoDirectorText);
		}
		if (SelectedEnemyLinkedText)
		{
			SelectedEnemyLinkedText->SetText(NoDirectorText);
		}
		if (SelectedEnemyInfoText)
		{
			SelectedEnemyInfoText->SetText(NoDirectorText);
		}
	}
}

void UGP_EncounterDebugRuntimeWidget::HandleMobClassSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SelectionType == ESelectInfo::Direct)
	{
		return;
	}

	if (AGP_EncounterDebugDirector* DebugDirector = GetDirector())
	{
		const int32 SelectedIndex = MobClassCombo ? MobClassCombo->FindOptionIndex(SelectedItem) : INDEX_NONE;
		if (DebugDirector->SelectAvailableMobClass(SelectedIndex))
		{
			SetStatusMessage(FString::Printf(TEXT("Class: %s"), *SelectedItem));
		}
	}
}

void UGP_EncounterDebugRuntimeWidget::HandleEnemySelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SelectionType == ESelectInfo::Direct)
	{
		return;
	}

	if (!EnemyCombo)
	{
		return;
	}

	const int32 SelectedIndex = EnemyCombo->FindOptionIndex(SelectedItem);
	if (!EnemyOptions.IsValidIndex(SelectedIndex))
	{
		return;
	}

	AGP_EnemyCharacter* Enemy = EnemyOptions[SelectedIndex].Get();
	if (AGP_EncounterDebugDirector* DebugDirector = GetDirector())
	{
		if (DebugDirector->SelectEnemy(Enemy))
		{
			SetStatusMessage(FString::Printf(TEXT("Selected: %s"), *GetNameSafe(Enemy)));
			RefreshSelectedEnemyInfo();
		}
	}
}

void UGP_EncounterDebugRuntimeWidget::HandleValidateClicked()
{
	if (AGP_EncounterDebugDirector* DebugDirector = GetDirector())
	{
		SetStatusMessage(DebugDirector->ValidateSetup() ? TEXT("Validate OK") : TEXT("Validate failed. Check Output Log."));
	}
}

void UGP_EncounterDebugRuntimeWidget::HandleStage1Clicked()
{
	if (AGP_EncounterDebugDirector* DebugDirector = GetDirector())
	{
		DebugDirector->SelectStage(0);
		SetStatusMessage(TEXT("Stage 1 selected"));
	}
}

void UGP_EncounterDebugRuntimeWidget::HandleStage2Clicked()
{
	if (AGP_EncounterDebugDirector* DebugDirector = GetDirector())
	{
		DebugDirector->SelectStage(1);
		SetStatusMessage(TEXT("Stage 2 selected"));
	}
}

void UGP_EncounterDebugRuntimeWidget::HandleStage3Clicked()
{
	if (AGP_EncounterDebugDirector* DebugDirector = GetDirector())
	{
		DebugDirector->SelectStage(2);
		SetStatusMessage(TEXT("Stage 3 selected"));
	}
}

void UGP_EncounterDebugRuntimeWidget::HandleTeleportClicked()
{
	if (AGP_EncounterDebugDirector* DebugDirector = GetDirector())
	{
		SetStatusMessage(DebugDirector->TeleportPlayerToSelectedStage() ? TEXT("Teleported") : TEXT("Teleport failed"));
	}
}

void UGP_EncounterDebugRuntimeWidget::HandleSpawnMobClicked()
{
	if (AGP_EncounterDebugDirector* DebugDirector = GetDirector())
	{
		SetStatusMessage(DebugDirector->SpawnSelectedStageMob() ? TEXT("Stage mob spawned") : TEXT("Stage mob spawn failed"));
		RefreshEnemyOptions();
	}
}

void UGP_EncounterDebugRuntimeWidget::HandleSpawnBossClicked()
{
	if (AGP_EncounterDebugDirector* DebugDirector = GetDirector())
	{
		SetStatusMessage(DebugDirector->SpawnSelectedStageBoss() ? TEXT("Stage boss spawned") : TEXT("Stage boss spawn failed"));
		RefreshEnemyOptions();
	}
}

void UGP_EncounterDebugRuntimeWidget::HandleSpawnSelectedClicked()
{
	if (AGP_EncounterDebugDirector* DebugDirector = GetDirector())
	{
		SetStatusMessage(DebugDirector->SpawnSelectedClass() ? TEXT("Selected spawned") : TEXT("Selected spawn failed"));
		RefreshEnemyOptions();
	}
}

void UGP_EncounterDebugRuntimeWidget::HandleRefreshClicked()
{
	if (AGP_EncounterDebugDirector* DebugDirector = GetDirector())
	{
		const int32 Count = DebugDirector->RefreshEnemyList().Num();
		SetStatusMessage(FString::Printf(TEXT("Enemy count: %d"), Count));
		RefreshEnemyOptions();
	}
}

void UGP_EncounterDebugRuntimeWidget::HandleReportSummaryClicked()
{
	ReportMode = EGPEncounterDebugReportMode::Summary;
	RefreshSelectedEnemyInfo();
}

void UGP_EncounterDebugRuntimeWidget::HandleReportAIClicked()
{
	ReportMode = EGPEncounterDebugReportMode::AI;
	RefreshSelectedEnemyInfo();
}

void UGP_EncounterDebugRuntimeWidget::HandleReportSkillClicked()
{
	ReportMode = EGPEncounterDebugReportMode::Skill;
	RefreshSelectedEnemyInfo();
}

void UGP_EncounterDebugRuntimeWidget::HandleReportGASClicked()
{
	ReportMode = EGPEncounterDebugReportMode::GAS;
	RefreshSelectedEnemyInfo();
}

void UGP_EncounterDebugRuntimeWidget::HandleReportMatadorClicked()
{
	ReportMode = EGPEncounterDebugReportMode::Matador;
	RefreshSelectedEnemyInfo();
}

void UGP_EncounterDebugRuntimeWidget::HandleReportAllClicked()
{
	ReportMode = EGPEncounterDebugReportMode::All;
	RefreshSelectedEnemyInfo();
}

void UGP_EncounterDebugRuntimeWidget::HandleClearClicked()
{
	if (AGP_EncounterDebugDirector* DebugDirector = GetDirector())
	{
		const int32 Count = DebugDirector->DespawnAllDebugEnemies();
		SetStatusMessage(FString::Printf(TEXT("Cleared: %d"), Count));
		RefreshEnemyOptions();
	}
}

void UGP_EncounterDebugRuntimeWidget::HandleKillClicked()
{
	if (AGP_EncounterDebugDirector* DebugDirector = GetDirector())
	{
		SetStatusMessage(DebugDirector->KillSelectedEnemy() ? TEXT("Kill requested") : TEXT("Kill failed"));
		RefreshEnemyOptions();
	}
}

void UGP_EncounterDebugRuntimeWidget::HandleDestroyClicked()
{
	if (AGP_EncounterDebugDirector* DebugDirector = GetDirector())
	{
		SetStatusMessage(DebugDirector->DestroySelectedEnemy() ? TEXT("Destroyed") : TEXT("Destroy failed"));
		RefreshEnemyOptions();
	}
}

void UGP_EncounterDebugRuntimeWidget::HandleHp100Clicked()
{
	if (AGP_EncounterDebugDirector* DebugDirector = GetDirector())
	{
		SetStatusMessage(DebugDirector->SetSelectedEnemyHealthPercent(1.0f) ? TEXT("HP 100") : TEXT("HP failed"));
		RefreshEnemyOptions();
	}
}

void UGP_EncounterDebugRuntimeWidget::HandleHp50Clicked()
{
	if (AGP_EncounterDebugDirector* DebugDirector = GetDirector())
	{
		SetStatusMessage(DebugDirector->SetSelectedEnemyHealthPercent(0.5f) ? TEXT("HP 50") : TEXT("HP failed"));
		RefreshEnemyOptions();
	}
}

void UGP_EncounterDebugRuntimeWidget::HandleHp1Clicked()
{
	if (AGP_EncounterDebugDirector* DebugDirector = GetDirector())
	{
		SetStatusMessage(DebugDirector->SetSelectedEnemyHealthPercent(0.01f) ? TEXT("HP 1") : TEXT("HP failed"));
		RefreshEnemyOptions();
	}
}

void UGP_EncounterDebugRuntimeWidget::HandleAIOnClicked()
{
	if (AGP_EncounterDebugDirector* DebugDirector = GetDirector())
	{
		SetStatusMessage(DebugDirector->SetSelectedEnemyAIEnabled(true) ? TEXT("AI On") : TEXT("AI failed"));
		RefreshSelectedEnemyInfo();
	}
}

void UGP_EncounterDebugRuntimeWidget::HandleAIOffClicked()
{
	if (AGP_EncounterDebugDirector* DebugDirector = GetDirector())
	{
		SetStatusMessage(DebugDirector->SetSelectedEnemyAIEnabled(false) ? TEXT("AI Off") : TEXT("AI failed"));
		RefreshSelectedEnemyInfo();
	}
}

void UGP_EncounterDebugRuntimeWidget::HandleCloseClicked()
{
	if (AGP_EncounterDebugDirector* DebugDirector = GetDirector())
	{
		DebugDirector->HideRuntimeDebugPanel();
	}
	else
	{
		RemoveFromParent();
	}
}
