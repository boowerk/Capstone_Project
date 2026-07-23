#include "UI/GP_SkillSelectWidget.h"

#include "AbilitySystem/Abilities/GP_SkillData.h"
#include "AbilitySystem/Abilities/GP_SkillPoolData.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "GameplayTags/GP_Tags.h"
#include "InputCoreTypes.h"
#include "Player/GP_PlayerController.h"
#include "Player/GP_PlayerState.h"
#include "UI/GP_SkillEntryWidget.h"

UGP_SkillSelectWidget::UGP_SkillSelectWidget(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, RuntimeFallbackBackplateTexture(
		FSoftObjectPath(
			TEXT("/Game/UI/HUD/Minimap/Textures/T_UI_Minimap_Backplate_DarkCircle.T_UI_Minimap_Backplate_DarkCircle")))
	, RuntimeFallbackRingTexture(
		FSoftObjectPath(
			TEXT("/Game/UI/HUD/Minimap/Textures/T_UI_Minimap_Ring_Brass.T_UI_Minimap_Ring_Brass")))
{
}

void UGP_SkillSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);
	SelectedSlotTag = GPTags::Ability::Skill::Slot01;
	EnsureRuntimeFallbackLayout();

	if (AGP_PlayerState* PlayerState = GetGPPlayerState())
	{
		PlayerState->OnEquippedSkillChanged.RemoveDynamic(this, &ThisClass::HandleEquippedSkillChanged);
		PlayerState->OnEquippedSkillChanged.AddDynamic(this, &ThisClass::HandleEquippedSkillChanged);
	}

	if (Button_Slot01)
	{
		Button_Slot01->OnClicked.RemoveDynamic(this, &ThisClass::HandleSlot01Clicked);
		Button_Slot01->OnClicked.AddDynamic(this, &ThisClass::HandleSlot01Clicked);
	}

	if (Button_Slot02)
	{
		Button_Slot02->OnClicked.RemoveDynamic(this, &ThisClass::HandleSlot02Clicked);
		Button_Slot02->OnClicked.AddDynamic(this, &ThisClass::HandleSlot02Clicked);
	}

	if (Button_Close)
	{
		Button_Close->OnClicked.RemoveDynamic(this, &ThisClass::HandleCloseClicked);
		Button_Close->OnClicked.AddDynamic(this, &ThisClass::HandleCloseClicked);
	}

	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		PlayerController->SetShowMouseCursor(true);

		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerController->SetInputMode(InputMode);
	}

	RefreshSkillSelection();
}

void UGP_SkillSelectWidget::NativeDestruct()
{
	if (AGP_PlayerState* PlayerState = GetGPPlayerState())
	{
		PlayerState->OnEquippedSkillChanged.RemoveDynamic(this, &ThisClass::HandleEquippedSkillChanged);
	}

	if (Button_Close)
	{
		Button_Close->OnClicked.RemoveDynamic(this, &ThisClass::HandleCloseClicked);
	}
	if (Button_Slot01)
	{
		Button_Slot01->OnClicked.RemoveDynamic(
			this,
			&ThisClass::HandleSlot01Clicked);
	}
	if (Button_Slot02)
	{
		Button_Slot02->OnClicked.RemoveDynamic(
			this,
			&ThisClass::HandleSlot02Clicked);
	}

	Super::NativeDestruct();
}

FReply UGP_SkillSelectWidget::NativeOnPreviewKeyDown(
	const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Q
		|| Key == EKeys::E
		|| Key == EKeys::K
		|| Key == EKeys::Escape)
	{
		return NativeOnKeyDown(InGeometry, InKeyEvent);
	}

	return Super::NativeOnPreviewKeyDown(
		InGeometry,
		InKeyEvent);
}

FReply UGP_SkillSelectWidget::NativeOnKeyDown(
	const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Q)
	{
		EquipSelectedSkillToSlot(GPTags::Ability::Skill::Slot01);
		return FReply::Handled();
	}

	if (Key == EKeys::E)
	{
		EquipSelectedSkillToSlot(GPTags::Ability::Skill::Slot02);
		return FReply::Handled();
	}

	if (Key == EKeys::K || Key == EKeys::Escape)
	{
		CloseSkillSelection();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UGP_SkillSelectWidget::RefreshSkillSelection()
{
	TArray<UGP_SkillData*> AvailableSkills = GetAvailableSkills();
	if (AvailableSkills.Num() > SkillWheelSlotCount)
	{
		if (!bOverflowWarningIssued)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("Skill selection pool on %s contains %d entries. Only the first %d are displayed."),
				*GetName(),
				AvailableSkills.Num(),
				SkillWheelSlotCount);
			bOverflowWarningIssued = true;
		}

		AvailableSkills.SetNum(SkillWheelSlotCount);
	}
	else
	{
		bOverflowWarningIssued = false;
	}

	UGP_SkillData* Slot01Skill = GetEquippedSkill(GPTags::Ability::Skill::Slot01);
	UGP_SkillData* Slot02Skill = GetEquippedSkill(GPTags::Ability::Skill::Slot02);

	if (!AvailableSkills.Contains(SelectedSkillData.Get()))
	{
		if (AvailableSkills.Contains(Slot01Skill))
		{
			SelectedSkillData = Slot01Skill;
		}
		else if (AvailableSkills.Contains(Slot02Skill))
		{
			SelectedSkillData = Slot02Skill;
		}
		else
		{
			SelectedSkillData =
				AvailableSkills.IsEmpty()
					? nullptr
					: AvailableSkills[0];
		}
	}

	RebuildSkillEntries(AvailableSkills, Slot01Skill, Slot02Skill);
	RefreshSlotTexts(Slot01Skill, Slot02Skill);
	RefreshSelectedSkillDetails();

	BP_OnSkillSelectionRefreshed(
		AvailableSkills,
		Slot01Skill,
		Slot02Skill,
		SelectedSlotTag);
}

TArray<UGP_SkillData*> UGP_SkillSelectWidget::GetAvailableSkills() const
{
	const AGP_PlayerController* PlayerController = GetGPPlayerController();
	const UGP_SkillPoolData* SkillPool = PlayerController ? PlayerController->GetSkillPoolData() : nullptr;
	return SkillPool ? SkillPool->GetSkills() : TArray<UGP_SkillData*>();
}

void UGP_SkillSelectWidget::SelectSlot01()
{
	SetSelectedSlot(GPTags::Ability::Skill::Slot01);
}

void UGP_SkillSelectWidget::SelectSlot02()
{
	SetSelectedSlot(GPTags::Ability::Skill::Slot02);
}

bool UGP_SkillSelectWidget::EquipSkillToSelectedSlot(UGP_SkillData* SkillData)
{
	return EquipSkillToSlot(SkillData, SelectedSlotTag);
}

bool UGP_SkillSelectWidget::EquipSkillToSlot(UGP_SkillData* SkillData, FGameplayTag SlotTag)
{
	if (!SlotTag.MatchesTagExact(GPTags::Ability::Skill::Slot01)
		&& !SlotTag.MatchesTagExact(GPTags::Ability::Skill::Slot02))
	{
		return false;
	}

	SelectedSlotTag = SlotTag;
	AGP_PlayerController* PlayerController = GetGPPlayerController();
	const bool bEquipped = PlayerController && PlayerController->RequestEquipSkill(SkillData, SlotTag);
	if (bEquipped)
	{
		RefreshSkillSelection();
	}
	return bEquipped;
}

bool UGP_SkillSelectWidget::EquipSelectedSkillToSlot(FGameplayTag SlotTag)
{
	return IsValid(SelectedSkillData.Get())
		&& EquipSkillToSlot(SelectedSkillData.Get(), SlotTag);
}

void UGP_SkillSelectWidget::CloseSkillSelection()
{
	if (AGP_PlayerController* PlayerController = GetGPPlayerController())
	{
		PlayerController->CloseSkillSelect();
		return;
	}

	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		PlayerController->SetShowMouseCursor(false);
		UWidgetBlueprintLibrary::SetInputMode_GameOnly(PlayerController);
	}

	RemoveFromParent();
}

UGP_SkillData* UGP_SkillSelectWidget::GetEquippedSkill(FGameplayTag SlotTag) const
{
	const AGP_PlayerState* PlayerState = GetGPPlayerState();
	return PlayerState ? PlayerState->GetEquippedSkillData(SlotTag) : nullptr;
}

FGameplayTag UGP_SkillSelectWidget::GetSlot01Tag()
{
	return GPTags::Ability::Skill::Slot01;
}

FGameplayTag UGP_SkillSelectWidget::GetSlot02Tag()
{
	return GPTags::Ability::Skill::Slot02;
}

void UGP_SkillSelectWidget::HandleEquippedSkillChanged(FGameplayTag SlotTag, UGP_SkillData* SkillData)
{
	RefreshSkillSelection();
}

void UGP_SkillSelectWidget::HandleSkillEntryClicked(UGP_SkillData* SkillData)
{
	SetSelectedSkill(SkillData);
}

void UGP_SkillSelectWidget::HandleSlot01Clicked()
{
	EquipSelectedSkillToSlot(GPTags::Ability::Skill::Slot01);
}

void UGP_SkillSelectWidget::HandleSlot02Clicked()
{
	EquipSelectedSkillToSlot(GPTags::Ability::Skill::Slot02);
}

void UGP_SkillSelectWidget::HandleCloseClicked()
{
	CloseSkillSelection();
}

AGP_PlayerController* UGP_SkillSelectWidget::GetGPPlayerController() const
{
	return Cast<AGP_PlayerController>(GetOwningPlayer());
}

AGP_PlayerState* UGP_SkillSelectWidget::GetGPPlayerState() const
{
	const AGP_PlayerController* PlayerController = GetGPPlayerController();
	return PlayerController ? PlayerController->GetPlayerState<AGP_PlayerState>() : nullptr;
}

void UGP_SkillSelectWidget::EnsureRuntimeFallbackLayout()
{
	if (CanvasPanel_SkillWheel || RuntimeFallbackRoot || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* HostCanvas =
		Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!HostCanvas)
	{
		TArray<UWidget*> AllWidgets;
		WidgetTree->GetAllWidgets(AllWidgets);
		for (UWidget* Widget : AllWidgets)
		{
			if (UCanvasPanel* CandidateCanvas =
				Cast<UCanvasPanel>(Widget))
			{
				HostCanvas = CandidateCanvas;
				break;
			}
		}
	}

	if (!HostCanvas)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Skill selection widget %s has no CanvasPanel host. Runtime wheel fallback could not be created."),
			*GetName());
		return;
	}

	// The runtime fallback fully replaces the legacy authored screen. Hide all
	// existing direct children before adding it so the old list, headings,
	// panels, and instructional copy cannot show through behind the wheel.
	for (int32 ChildIndex = 0;
		ChildIndex < HostCanvas->GetChildrenCount();
		++ChildIndex)
	{
		if (UWidget* LegacyChild =
			HostCanvas->GetChildAt(ChildIndex))
		{
			LegacyChild->SetVisibility(
				ESlateVisibility::Collapsed);
		}
	}

	if (VerticalBox_SkillList)
	{
		VerticalBox_SkillList->SetVisibility(
			ESlateVisibility::Collapsed);
	}
	if (Button_Slot01)
	{
		Button_Slot01->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (Button_Slot02)
	{
		Button_Slot02->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (Button_Close)
	{
		Button_Close->SetVisibility(ESlateVisibility::Collapsed);
	}

	RuntimeFallbackRoot =
		WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(),
			TEXT("RuntimeSkillSelectFallbackRoot"));
	if (!RuntimeFallbackRoot)
	{
		return;
	}

	UCanvasPanelSlot* FallbackRootSlot =
		HostCanvas->AddChildToCanvas(RuntimeFallbackRoot);
	if (!FallbackRootSlot)
	{
		RuntimeFallbackRoot = nullptr;
		return;
	}
	FallbackRootSlot->SetAnchors(
		FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	FallbackRootSlot->SetOffsets(FMargin(0.0f));
	FallbackRootSlot->SetZOrder(100);

	auto AddFillWidget =
		[this](UWidget* Widget, int32 ZOrder)
		{
			if (!RuntimeFallbackRoot || !Widget)
			{
				return static_cast<UCanvasPanelSlot*>(nullptr);
			}

			UCanvasPanelSlot* Slot =
				RuntimeFallbackRoot->AddChildToCanvas(Widget);
			if (Slot)
			{
				Slot->SetAnchors(
					FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
				Slot->SetOffsets(FMargin(0.0f));
				Slot->SetZOrder(ZOrder);
			}
			return Slot;
		};

	auto AddCenteredWidget =
		[this](UWidget* Widget, const FVector2D& Size, int32 ZOrder)
		{
			if (!RuntimeFallbackRoot || !Widget)
			{
				return static_cast<UCanvasPanelSlot*>(nullptr);
			}

			UCanvasPanelSlot* Slot =
				RuntimeFallbackRoot->AddChildToCanvas(Widget);
			if (Slot)
			{
				Slot->SetAnchors(FAnchors(0.5f, 0.5f));
				Slot->SetAlignment(FVector2D(0.5f, 0.5f));
				Slot->SetPosition(FVector2D::ZeroVector);
				Slot->SetSize(Size);
				Slot->SetZOrder(ZOrder);
			}
			return Slot;
		};

	auto ConfigureCenteredText =
		[](UTextBlock* TextBlock, int32 FontSize)
		{
			if (!TextBlock)
			{
				return;
			}

			FSlateFontInfo Font = TextBlock->GetFont();
			Font.Size = FontSize;
			TextBlock->SetFont(Font);
			TextBlock->SetJustification(
				ETextJustify::Center);
			TextBlock->SetColorAndOpacity(
				FSlateColor(
					FLinearColor(
						0.86f,
						0.78f,
						0.61f,
						1.0f)));
		};

	UBorder* ScreenScrim =
		WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			TEXT("RuntimeSkillSelectScrim"));
	ScreenScrim->SetBrushColor(
		FLinearColor(0.008f, 0.007f, 0.006f, 0.90f));
	AddFillWidget(ScreenScrim, 0);

	auto AddFrameEdge =
		[this](
			const FName EdgeName,
			const FAnchors& Anchors,
			const FMargin& Offsets)
		{
			UBorder* Edge =
				WidgetTree->ConstructWidget<UBorder>(
					UBorder::StaticClass(),
					EdgeName);
			Edge->SetBrushColor(
				FLinearColor(0.48f, 0.35f, 0.16f, 0.52f));
			Edge->SetVisibility(
				ESlateVisibility::SelfHitTestInvisible);

			if (UCanvasPanelSlot* EdgeSlot =
				RuntimeFallbackRoot->AddChildToCanvas(Edge))
			{
				EdgeSlot->SetAnchors(Anchors);
				EdgeSlot->SetOffsets(Offsets);
				EdgeSlot->SetZOrder(9);
			}
		};

	AddFrameEdge(
		TEXT("RuntimeSkillSelectFrameTop"),
		FAnchors(0.0f, 0.0f, 1.0f, 0.0f),
		FMargin(18.0f, 18.0f, 18.0f, 1.0f));
	AddFrameEdge(
		TEXT("RuntimeSkillSelectFrameBottom"),
		FAnchors(0.0f, 1.0f, 1.0f, 1.0f),
		FMargin(18.0f, -19.0f, 18.0f, 1.0f));
	AddFrameEdge(
		TEXT("RuntimeSkillSelectFrameLeft"),
		FAnchors(0.0f, 0.0f, 0.0f, 1.0f),
		FMargin(18.0f, 18.0f, 1.0f, 18.0f));
	AddFrameEdge(
		TEXT("RuntimeSkillSelectFrameRight"),
		FAnchors(1.0f, 0.0f, 1.0f, 1.0f),
		FMargin(-19.0f, 18.0f, 1.0f, 18.0f));

	UTextBlock* TitleText =
		WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			TEXT("RuntimeSkillSelectTitle"));
	ConfigureCenteredText(TitleText, 28);
	TitleText->SetText(
		NSLOCTEXT(
			"SkillSelection",
			"SkillEquipTitle",
			"스킬 장착"));
	TitleText->SetVisibility(
		ESlateVisibility::SelfHitTestInvisible);
	if (UCanvasPanelSlot* TitleSlot =
		RuntimeFallbackRoot->AddChildToCanvas(TitleText))
	{
		TitleSlot->SetAnchors(FAnchors(0.5f, 0.0f));
		TitleSlot->SetAlignment(FVector2D(0.5f, 0.0f));
		TitleSlot->SetPosition(FVector2D(0.0f, 26.0f));
		TitleSlot->SetSize(FVector2D(360.0f, 44.0f));
		TitleSlot->SetZOrder(8);
	}

	Button_Close =
		WidgetTree->ConstructWidget<UButton>(
			UButton::StaticClass(),
			TEXT("Button_Close_Runtime"));
	Button_Close->SetBackgroundColor(
		FLinearColor(0.08f, 0.07f, 0.05f, 0.92f));
	UTextBlock* CloseText =
		WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			TEXT("Text_Close_Runtime"));
	ConfigureCenteredText(CloseText, 13);
	CloseText->SetText(
		NSLOCTEXT(
			"SkillSelection",
			"CloseSkillWheel",
			"닫기  K / Esc"));
	Button_Close->AddChild(CloseText);
	if (UCanvasPanelSlot* CloseSlot =
		RuntimeFallbackRoot->AddChildToCanvas(Button_Close))
	{
		CloseSlot->SetAnchors(FAnchors(1.0f, 0.0f));
		CloseSlot->SetAlignment(FVector2D(1.0f, 0.0f));
		CloseSlot->SetPosition(FVector2D(-28.0f, 28.0f));
		CloseSlot->SetSize(FVector2D(156.0f, 42.0f));
		CloseSlot->SetZOrder(8);
	}

	UImage* BrassRing =
		WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(),
			TEXT("RuntimeSkillSelectBrassRing"));
	if (UTexture2D* RingTexture =
			RuntimeFallbackRingTexture.LoadSynchronous())
	{
		BrassRing->SetBrushFromTexture(RingTexture, true);
	}
	BrassRing->SetColorAndOpacity(
		FLinearColor(0.72f, 0.57f, 0.30f, 0.72f));
	BrassRing->SetVisibility(
		ESlateVisibility::SelfHitTestInvisible);
	AddCenteredWidget(
		BrassRing,
		FVector2D(640.0f, 640.0f),
		1);

	UImage* DarkBackplate =
		WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(),
			TEXT("RuntimeSkillSelectDarkBackplate"));
	if (UTexture2D* BackplateTexture =
			RuntimeFallbackBackplateTexture.LoadSynchronous())
	{
		DarkBackplate->SetBrushFromTexture(
			BackplateTexture,
			true);
	}
	DarkBackplate->SetColorAndOpacity(
		FLinearColor(0.78f, 0.76f, 0.70f, 0.94f));
	DarkBackplate->SetVisibility(
		ESlateVisibility::SelfHitTestInvisible);
	AddCenteredWidget(
		DarkBackplate,
		FVector2D(390.0f, 390.0f),
		2);

	constexpr float DividerOuterRadius = 314.0f;
	constexpr float DividerInnerRadius = 202.0f;
	constexpr float DividerLength =
		DividerOuterRadius - DividerInnerRadius;
	constexpr float DividerMidRadius =
		(DividerOuterRadius + DividerInnerRadius) * 0.5f;
	constexpr float SectorAngleDegrees =
		360.0f / SkillWheelSlotCount;

	for (int32 Index = 0; Index < SkillWheelSlotCount; ++Index)
	{
		const float BoundaryAngleDegrees =
			SkillWheelStartAngleDegrees
			+ (static_cast<float>(Index) + 0.5f)
				* SectorAngleDegrees;
		const float BoundaryAngleRadians =
			FMath::DegreesToRadians(BoundaryAngleDegrees);

		UBorder* Divider =
			WidgetTree->ConstructWidget<UBorder>(
				UBorder::StaticClass(),
				FName(
					*FString::Printf(
						TEXT("RuntimeSkillSelectDivider_%02d"),
						Index)));
		Divider->SetBrushColor(
			FLinearColor(0.49f, 0.36f, 0.17f, 0.58f));
		Divider->SetVisibility(
			ESlateVisibility::SelfHitTestInvisible);
		Divider->SetRenderTransformPivot(
			FVector2D(0.5f, 0.5f));
		Divider->SetRenderTransformAngle(
			BoundaryAngleDegrees);

		if (UCanvasPanelSlot* DividerSlot =
			RuntimeFallbackRoot->AddChildToCanvas(Divider))
		{
			DividerSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			DividerSlot->SetAlignment(
				FVector2D(0.5f, 0.5f));
			DividerSlot->SetPosition(
				FVector2D(
					FMath::Cos(BoundaryAngleRadians),
					FMath::Sin(BoundaryAngleRadians))
				* DividerMidRadius);
			DividerSlot->SetSize(
				FVector2D(DividerLength, 1.25f));
			DividerSlot->SetZOrder(3);
		}
	}

	CanvasPanel_SkillWheel =
		WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(),
			TEXT("CanvasPanel_SkillWheel_Runtime"));
	AddFillWidget(CanvasPanel_SkillWheel, 4);

	USizeBox* CenterHost =
		WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			TEXT("RuntimeSkillSelectCenterHost"));
	CenterHost->SetWidthOverride(330.0f);
	CenterHost->SetHeightOverride(330.0f);
	AddCenteredWidget(
		CenterHost,
		FVector2D(330.0f, 330.0f),
		5);

	UVerticalBox* CenterColumn =
		WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(),
			TEXT("RuntimeSkillSelectCenterColumn"));
	CenterHost->AddChild(CenterColumn);

	USizeBox* SelectedIconHost =
		WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			TEXT("RuntimeSelectedSkillIconHost"));
	SelectedIconHost->SetWidthOverride(128.0f);
	SelectedIconHost->SetHeightOverride(128.0f);
	Image_SelectedSkill =
		WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(),
			TEXT("Image_SelectedSkill_Runtime"));
	SelectedIconHost->AddChild(Image_SelectedSkill);
	if (UVerticalBoxSlot* IconSlot =
		CenterColumn->AddChildToVerticalBox(SelectedIconHost))
	{
		IconSlot->SetHorizontalAlignment(HAlign_Center);
		IconSlot->SetPadding(FMargin(0.0f, 8.0f));
	}

	Text_SelectedSkillName =
		WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			TEXT("Text_SelectedSkillName_Runtime"));
	ConfigureCenteredText(Text_SelectedSkillName, 24);
	if (UVerticalBoxSlot* NameSlot =
		CenterColumn->AddChildToVerticalBox(
			Text_SelectedSkillName))
	{
		NameSlot->SetHorizontalAlignment(HAlign_Fill);
		NameSlot->SetPadding(FMargin(4.0f));
	}

	Text_SelectedSkillDescription =
		WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			TEXT("Text_SelectedSkillDescription_Runtime"));
	ConfigureCenteredText(
		Text_SelectedSkillDescription,
		14);
	Text_SelectedSkillDescription->SetAutoWrapText(true);
	Text_SelectedSkillDescription->SetWrapTextAt(300.0f);
	if (UVerticalBoxSlot* DescriptionSlot =
		CenterColumn->AddChildToVerticalBox(
			Text_SelectedSkillDescription))
	{
		DescriptionSlot->SetHorizontalAlignment(HAlign_Fill);
		DescriptionSlot->SetPadding(FMargin(8.0f, 4.0f));
	}

	Text_SelectedSkillCooldown =
		WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			TEXT("Text_SelectedSkillCooldown_Runtime"));
	ConfigureCenteredText(
		Text_SelectedSkillCooldown,
		15);
	if (UVerticalBoxSlot* CooldownSlot =
		CenterColumn->AddChildToVerticalBox(
			Text_SelectedSkillCooldown))
	{
		CooldownSlot->SetHorizontalAlignment(HAlign_Fill);
		CooldownSlot->SetPadding(FMargin(4.0f, 8.0f));
	}

	UBorder* SlotButtonsPanel =
		WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			TEXT("RuntimeSkillSelectSlotButtonsPanel"));
	SlotButtonsPanel->SetBrushColor(
		FLinearColor(0.035f, 0.028f, 0.018f, 0.94f));
	SlotButtonsPanel->SetPadding(FMargin(4.0f));
	if (UCanvasPanelSlot* SlotButtonsPanelSlot =
		RuntimeFallbackRoot->AddChildToCanvas(
			SlotButtonsPanel))
	{
		SlotButtonsPanelSlot->SetAnchors(
			FAnchors(0.5f, 1.0f));
		SlotButtonsPanelSlot->SetAlignment(
			FVector2D(0.5f, 1.0f));
		SlotButtonsPanelSlot->SetPosition(
			FVector2D(0.0f, -38.0f));
		SlotButtonsPanelSlot->SetSize(
			FVector2D(420.0f, 55.0f));
		SlotButtonsPanelSlot->SetZOrder(7);
	}

	UHorizontalBox* SlotButtons =
		WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(),
			TEXT("RuntimeSkillSelectSlotButtons"));
	SlotButtonsPanel->AddChild(SlotButtons);

	auto AddEquipButton =
		[this, SlotButtons, &ConfigureCenteredText](
			const FName ButtonName,
			const FName TextName,
			UButton*& OutButton,
			UTextBlock*& OutText)
		{
			OutButton =
				WidgetTree->ConstructWidget<UButton>(
					UButton::StaticClass(),
					ButtonName);
			OutButton->SetBackgroundColor(
				FLinearColor(
					0.16f,
					0.12f,
					0.06f,
					0.96f));
			OutText =
				WidgetTree->ConstructWidget<UTextBlock>(
					UTextBlock::StaticClass(),
					TextName);
			ConfigureCenteredText(OutText, 15);
			OutButton->AddChild(OutText);

			USizeBox* ButtonHost =
				WidgetTree->ConstructWidget<USizeBox>(
					USizeBox::StaticClass());
			ButtonHost->SetWidthOverride(198.0f);
			ButtonHost->SetHeightOverride(47.0f);
			ButtonHost->AddChild(OutButton);
			if (UHorizontalBoxSlot* ButtonSlot =
				SlotButtons->AddChildToHorizontalBox(ButtonHost))
			{
				ButtonSlot->SetPadding(
					FMargin(4.0f, 0.0f));
				ButtonSlot->SetHorizontalAlignment(HAlign_Center);
				ButtonSlot->SetVerticalAlignment(VAlign_Center);
			}
		};

	UButton* RuntimeSlot01Button = nullptr;
	UTextBlock* RuntimeSlot01Text = nullptr;
	AddEquipButton(
		TEXT("Button_Slot01_Runtime"),
		TEXT("Text_Slot01_Runtime"),
		RuntimeSlot01Button,
		RuntimeSlot01Text);
	Button_Slot01 = RuntimeSlot01Button;
	Text_Slot01 = RuntimeSlot01Text;

	UButton* RuntimeSlot02Button = nullptr;
	UTextBlock* RuntimeSlot02Text = nullptr;
	AddEquipButton(
		TEXT("Button_Slot02_Runtime"),
		TEXT("Text_Slot02_Runtime"),
		RuntimeSlot02Button,
		RuntimeSlot02Text);
	Button_Slot02 = RuntimeSlot02Button;
	Text_Slot02 = RuntimeSlot02Text;

	RuntimeFallbackRoot->InvalidateLayoutAndVolatility();
	HostCanvas->InvalidateLayoutAndVolatility();
}

void UGP_SkillSelectWidget::SetSelectedSlot(FGameplayTag SlotTag)
{
	if (!SlotTag.MatchesTagExact(GPTags::Ability::Skill::Slot01)
		&& !SlotTag.MatchesTagExact(GPTags::Ability::Skill::Slot02))
	{
		return;
	}

	SelectedSlotTag = SlotTag;
	RefreshSkillSelection();
}

void UGP_SkillSelectWidget::SetSelectedSkill(UGP_SkillData* SkillData)
{
	if (!IsValid(SkillData))
	{
		return;
	}

	SelectedSkillData = SkillData;
	RefreshSkillEntryStates(
		GetEquippedSkill(GPTags::Ability::Skill::Slot01),
		GetEquippedSkill(GPTags::Ability::Skill::Slot02));
	RefreshSelectedSkillDetails();
}

void UGP_SkillSelectWidget::RebuildSkillEntries(const TArray<UGP_SkillData*>& AvailableSkills, UGP_SkillData* Slot01Skill, UGP_SkillData* Slot02Skill)
{
	if (CanvasPanel_SkillWheel)
	{
		CanvasPanel_SkillWheel->ClearChildren();
	}
	if (VerticalBox_SkillList)
	{
		VerticalBox_SkillList->ClearChildren();
	}
	SkillEntryWidgets.Reset();

	if (!SkillEntryWidgetClass)
	{
		if (!bMissingEntryClassWarningIssued)
		{
			UE_LOG(
				LogTemp,
				Error,
				TEXT("Skill selection widget %s has no SkillEntryWidgetClass. The eight-slot wheel cannot create entries."),
				*GetName());
			bMissingEntryClassWarningIssued = true;
		}
		return;
	}
	bMissingEntryClassWarningIssued = false;

	auto CreateSkillEntry =
		[this, Slot01Skill, Slot02Skill](UGP_SkillData* SkillData)
		{
			UGP_SkillEntryWidget* EntryWidget =
				CreateWidget<UGP_SkillEntryWidget>(
					GetOwningPlayer(),
					SkillEntryWidgetClass);
			if (!IsValid(EntryWidget))
			{
				return static_cast<UGP_SkillEntryWidget*>(nullptr);
			}

			EntryWidget->SetupSkillEntry(SkillData);
			EntryWidget->SetCompactWheelMode(
				CanvasPanel_SkillWheel != nullptr);
			EntryWidget->SetEquipped(
				IsValid(SkillData)
				&& (SkillData == Slot01Skill || SkillData == Slot02Skill));
			EntryWidget->SetSelected(
				IsValid(SkillData)
				&& SkillData == SelectedSkillData.Get());
			EntryWidget->OnSkillEntryClicked.RemoveDynamic(
				this,
				&ThisClass::HandleSkillEntryClicked);
			EntryWidget->OnSkillEntryClicked.AddDynamic(
				this,
				&ThisClass::HandleSkillEntryClicked);
			SkillEntryWidgets.Add(EntryWidget);
			return EntryWidget;
		};

	if (CanvasPanel_SkillWheel)
	{
		for (int32 Index = 0; Index < SkillWheelSlotCount; ++Index)
		{
			UGP_SkillData* SkillData =
				AvailableSkills.IsValidIndex(Index)
					? AvailableSkills[Index]
					: nullptr;
			UGP_SkillEntryWidget* EntryWidget = CreateSkillEntry(SkillData);
			if (!IsValid(EntryWidget))
			{
				continue;
			}

			UWidget* WheelChild = EntryWidget;
			if (WidgetTree)
			{
				USizeBox* EntryHost =
					WidgetTree->ConstructWidget<USizeBox>(
						USizeBox::StaticClass());
				if (EntryHost)
				{
					EntryHost->SetWidthOverride(
						SkillWheelEntrySize);
					EntryHost->SetHeightOverride(
						SkillWheelEntrySize);
					EntryHost->SetClipping(
						EWidgetClipping::ClipToBounds);
					EntryHost->AddChild(EntryWidget);
					WheelChild = EntryHost;
				}
			}

			UCanvasPanelSlot* CanvasSlot =
				CanvasPanel_SkillWheel->AddChildToCanvas(WheelChild);
			if (!CanvasSlot)
			{
				continue;
			}

			const float AngleDegrees =
				SkillWheelStartAngleDegrees
				+ Index * (360.0f / SkillWheelSlotCount);
			const float AngleRadians = FMath::DegreesToRadians(AngleDegrees);
			CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CanvasSlot->SetAutoSize(true);
			CanvasSlot->SetPosition(
				FVector2D(
					FMath::Cos(AngleRadians),
					FMath::Sin(AngleRadians))
				* SkillWheelRadius);
		}
		return;
	}

	if (!VerticalBox_SkillList)
	{
		return;
	}

	for (UGP_SkillData* SkillData : AvailableSkills)
	{
		if (UGP_SkillEntryWidget* EntryWidget = CreateSkillEntry(SkillData))
		{
			VerticalBox_SkillList->AddChildToVerticalBox(EntryWidget);
		}
	}
}

void UGP_SkillSelectWidget::RefreshSkillEntryStates(
	UGP_SkillData* Slot01Skill,
	UGP_SkillData* Slot02Skill)
{
	for (UGP_SkillEntryWidget* EntryWidget : SkillEntryWidgets)
	{
		if (!IsValid(EntryWidget))
		{
			continue;
		}

		UGP_SkillData* SkillData = EntryWidget->GetSkillData();
		EntryWidget->SetEquipped(
			IsValid(SkillData)
				&& (SkillData == Slot01Skill || SkillData == Slot02Skill));
		EntryWidget->SetSelected(
			IsValid(SkillData)
				&& SkillData == SelectedSkillData.Get());
	}
}

void UGP_SkillSelectWidget::RefreshSelectedSkillDetails() const
{
	const UGP_SkillData* SkillData = SelectedSkillData.Get();
	const bool bHasSkill = IsValid(SkillData);

	if (Image_SelectedSkill)
	{
		UTexture2D* IconTexture =
			bHasSkill
				? SkillData->SkillIcon.LoadSynchronous()
				: nullptr;
		Image_SelectedSkill->SetVisibility(
			IsValid(IconTexture)
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Hidden);
		if (IsValid(IconTexture))
		{
			Image_SelectedSkill->SetBrushFromTexture(IconTexture, true);
		}
	}

	if (Text_SelectedSkillName)
	{
		Text_SelectedSkillName->SetText(
			bHasSkill
				? SkillData->SkillName
				: NSLOCTEXT(
					"SkillSelection",
					"NoSelectedSkill",
					"스킬을 선택하세요"));
	}

	if (Text_SelectedSkillDescription)
	{
		Text_SelectedSkillDescription->SetText(
			bHasSkill
				? SkillData->SkillDescription
				: FText::GetEmpty());
	}

	if (Text_SelectedSkillCooldown)
	{
		Text_SelectedSkillCooldown->SetText(GetCooldownDisplayText(SkillData));
	}
}

void UGP_SkillSelectWidget::RefreshSlotTexts(UGP_SkillData* Slot01Skill, UGP_SkillData* Slot02Skill) const
{
	if (Text_Slot01)
	{
		Text_Slot01->SetText(GetSlotDisplayText(TEXT("Q"), Slot01Skill));
	}

	if (Text_Slot02)
	{
		Text_Slot02->SetText(GetSlotDisplayText(TEXT("E"), Slot02Skill));
	}
}

FText UGP_SkillSelectWidget::GetSlotDisplayText(const TCHAR* SlotLabel, const UGP_SkillData* SkillData) const
{
	if (!IsValid(SkillData) || SkillData->SkillName.IsEmpty())
	{
		return FText::Format(
			NSLOCTEXT(
				"SkillSelection",
				"EmptySlotFormat",
				"{0} : 비어 있음"),
			FText::FromString(SlotLabel));
	}

	return FText::Format(NSLOCTEXT("SkillSelection", "EquippedSlotFormat", "{0} : {1}"), FText::FromString(SlotLabel), SkillData->SkillName);
}

FText UGP_SkillSelectWidget::GetCooldownDisplayText(
	const UGP_SkillData* SkillData) const
{
	if (!IsValid(SkillData))
	{
		return FText::GetEmpty();
	}

	switch (SkillData->CooldownPolicy)
	{
	case EGP_CooldownPolicy::None:
		return NSLOCTEXT(
			"SkillSelection",
			"NoCooldown",
			"재사용 대기시간 없음");
	case EGP_CooldownPolicy::Generic:
		return FText::Format(
			NSLOCTEXT(
				"SkillSelection",
				"CooldownSeconds",
				"재사용 {0}초"),
			FText::AsNumber(SkillData->CooldownDuration));
	case EGP_CooldownPolicy::Custom:
	default:
		return NSLOCTEXT(
			"SkillSelection",
			"CustomCooldown",
			"고유 재사용 대기시간");
	}
}
