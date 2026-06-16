#include "UI/GP_SkillEntryWidget.h"

#include "AbilitySystem/Abilities/GP_SkillData.h"
#include "Components/Button.h"
#include "Components/Image.h"
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
	RefreshView();
}

void UGP_SkillEntryWidget::SetEquipped(bool bNewEquipped)
{
	if (Text_Equipped)
	{
		Text_Equipped->SetVisibility(bNewEquipped ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UGP_SkillEntryWidget::HandleRootClicked()
{
	if (SkillData)
	{
		OnSkillEntryClicked.Broadcast(SkillData.Get());
	}
}

void UGP_SkillEntryWidget::RefreshView()
{
	const bool bHasSkill = IsValid(SkillData);

	if (Text_Name)
	{
		Text_Name->SetText(bHasSkill ? SkillData->SkillName : FText::GetEmpty());
	}

	if (Text_Description)
	{
		Text_Description->SetText(bHasSkill ? SkillData->SkillDescription : FText::GetEmpty());
	}

	if (Image_Icon)
	{
		UTexture2D* IconTexture = bHasSkill ? SkillData->SkillIcon.LoadSynchronous() : nullptr;
		Image_Icon->SetVisibility(IsValid(IconTexture) ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		if (IsValid(IconTexture))
		{
			Image_Icon->SetBrushFromTexture(IconTexture, true);
		}
	}

	SetEquipped(false);
}
