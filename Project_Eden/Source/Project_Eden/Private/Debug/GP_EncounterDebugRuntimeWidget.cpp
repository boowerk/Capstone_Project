#include "Debug/GP_EncounterDebugRuntimeWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ComboBoxString.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
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

void UGP_EncounterDebugRuntimeWidget::BuildPanel()
{
	UCanvasPanel* ScreenRoot = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("EncounterDebugScreenRoot"));
	WidgetTree->RootWidget = ScreenRoot;

	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("EncounterDebugPanelSize"));
	PanelSize->SetWidthOverride(360.0f);
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
		return;
	}

	const TArray<FGPEncounterDebugEnemySnapshot> Snapshots = DebugDirector->RefreshEnemyList();
	if (Snapshots.Num() <= 0)
	{
		EnemyCombo->AddOption(TEXT("<No Enemies>"));
		EnemyCombo->SetSelectedIndex(0);
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
	}
}

void UGP_EncounterDebugRuntimeWidget::HandleAIOffClicked()
{
	if (AGP_EncounterDebugDirector* DebugDirector = GetDirector())
	{
		SetStatusMessage(DebugDirector->SetSelectedEnemyAIEnabled(false) ? TEXT("AI Off") : TEXT("AI failed"));
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
