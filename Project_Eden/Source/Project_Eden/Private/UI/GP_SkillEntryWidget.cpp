#include "UI/GP_SkillEntryWidget.h"

#include "AbilitySystem/Abilities/GP_SkillData.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UGP_SkillEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Root)
	{
		Button_Root->OnClicked.RemoveDynamic(this, &ThisClass::HandleRootClicked);
		Button_Root->OnClicked.AddDynamic(this, &ThisClass::HandleRootClicked);
	}

	RefreshView();
}

void UGP_SkillEntryWidget::SetupSkillEntry(UGP_SkillData* NewSkillData)
{
	SkillData = NewSkillData;
	bIsEquipped = false;
	bIsSelected = false;
	RefreshView();
}

void UGP_SkillEntryWidget::SetEquipped(bool bNewEquipped)
{
	bIsEquipped = bNewEquipped;
	RefreshStateVisuals();
}

void UGP_SkillEntryWidget::SetSelected(bool bNewSelected)
{
	bIsSelected = bNewSelected;
	RefreshStateVisuals();
}

void UGP_SkillEntryWidget::SetCompactWheelMode(
	bool bNewCompactWheelMode)
{
	bCompactWheelMode = bNewCompactWheelMode;
	if (bCompactWheelMode)
	{
		EnsureCompactWheelLayout();
	}
	RefreshView();
}

void UGP_SkillEntryWidget::HandleRootClicked()
{
	if (SkillData)
	{
		OnSkillEntryClicked.Broadcast(SkillData.Get());
	}
}

void UGP_SkillEntryWidget::EnsureCompactWheelLayout()
{
	if (bCompactWheelLayoutBuilt || !Button_Root || !WidgetTree)
	{
		return;
	}

	// The authored list entry has offsets intended for a horizontal list.
	// Replace only its button content in wheel mode so the icon is guaranteed
	// to sit at the exact center of the square radial slot.
	Button_Root->ClearChildren();

	UOverlay* IconOverlay =
		WidgetTree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(),
			TEXT("RuntimeCompactSkillIconOverlay"));
	if (!IconOverlay)
	{
		return;
	}
	Button_Root->AddChild(IconOverlay);

	USizeBox* IconHost =
		WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			TEXT("RuntimeCompactSkillIconHost"));
	if (!IconHost)
	{
		return;
	}
	IconHost->SetWidthOverride(96.0f);
	IconHost->SetHeightOverride(96.0f);

	if (UOverlaySlot* IconHostSlot =
		IconOverlay->AddChildToOverlay(IconHost))
	{
		IconHostSlot->SetHorizontalAlignment(HAlign_Center);
		IconHostSlot->SetVerticalAlignment(VAlign_Center);
	}

	Image_Icon =
		WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(),
			TEXT("Image_Icon_RuntimeCompact"));
	if (!Image_Icon)
	{
		return;
	}
	Image_Icon->SetVisibility(
		ESlateVisibility::SelfHitTestInvisible);
	IconHost->AddChild(Image_Icon);

	bCompactWheelLayoutBuilt = true;
}

void UGP_SkillEntryWidget::RefreshView()
{
	const bool bHasSkill = IsValid(SkillData);

	if (Button_Root)
	{
		Button_Root->SetIsEnabled(bHasSkill);
	}

	if (Text_Name)
	{
		Text_Name->SetText(
			bHasSkill
				? SkillData->SkillName
				: NSLOCTEXT("SkillSelection", "EmptyWheelSlot", "빈 슬롯"));
		if (bCompactWheelMode)
		{
			// The authored entry places its label beside the icon. That layout
			// does not fit a square radial slot and causes long skill names to
			// spill sideways. The wheel is intentionally icon-only; the full
			// name remains visible in the center detail panel.
			Text_Name->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			Text_Name->SetVisibility(
				ESlateVisibility::SelfHitTestInvisible);
			Text_Name->SetAutoWrapText(false);
		}
	}

	if (Text_Description)
	{
		Text_Description->SetText(bHasSkill ? SkillData->SkillDescription : FText::GetEmpty());
		Text_Description->SetVisibility(
			bCompactWheelMode || !bHasSkill
				? ESlateVisibility::Collapsed
				: ESlateVisibility::SelfHitTestInvisible);
	}

	if (Image_Icon)
	{
		UTexture2D* IconTexture = bHasSkill ? SkillData->SkillIcon.LoadSynchronous() : nullptr;
		Image_Icon->SetVisibility(IsValid(IconTexture) ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
		if (IsValid(IconTexture))
		{
			Image_Icon->SetBrushFromTexture(IconTexture, true);
		}
		else
		{
			Image_Icon->SetBrushFromTexture(nullptr);
		}
	}

	RefreshStateVisuals();
}

void UGP_SkillEntryWidget::RefreshStateVisuals()
{
	const bool bHasSkill = IsValid(SkillData);
	if (Text_Equipped)
	{
		Text_Equipped->SetVisibility(
			bHasSkill && bIsEquipped
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed);
	}

	if (Image_Selected)
	{
		Image_Selected->SetVisibility(
			bHasSkill && bIsSelected
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed);
	}

	if (Button_Root)
	{
		const FLinearColor ButtonColor =
			!bHasSkill
				? FLinearColor(0.025f, 0.022f, 0.018f, 0.45f)
				: bIsSelected
					? FLinearColor(0.72f, 0.50f, 0.17f, 1.0f)
					: bIsEquipped
						? FLinearColor(0.30f, 0.22f, 0.09f, 0.96f)
						: FLinearColor(0.08f, 0.07f, 0.055f, 0.92f);
		Button_Root->SetBackgroundColor(ButtonColor);
	}

	SetRenderOpacity(bHasSkill ? 1.0f : 0.42f);
	SetRenderScale(FVector2D(1.0f, 1.0f));
}
