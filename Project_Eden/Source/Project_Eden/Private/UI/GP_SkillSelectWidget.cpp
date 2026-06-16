#include "UI/GP_SkillSelectWidget.h"

#include "AbilitySystem/Abilities/GP_SkillData.h"
#include "AbilitySystem/Abilities/GP_SkillPoolData.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "GameplayTags/GP_Tags.h"
#include "Player/GP_PlayerController.h"
#include "Player/GP_PlayerState.h"
#include "UI/GP_SkillEntryWidget.h"

void UGP_SkillSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SelectedSlotTag = GPTags::Ability::Skill::Slot01;

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

	RefreshSkillSelection();
}

void UGP_SkillSelectWidget::NativeDestruct()
{
	if (AGP_PlayerState* PlayerState = GetGPPlayerState())
	{
		PlayerState->OnEquippedSkillChanged.RemoveDynamic(this, &ThisClass::HandleEquippedSkillChanged);
	}

	Super::NativeDestruct();
}

void UGP_SkillSelectWidget::RefreshSkillSelection()
{
	TArray<UGP_SkillData*> AvailableSkills = GetAvailableSkills();
	UGP_SkillData* Slot01Skill = GetEquippedSkill(GPTags::Ability::Skill::Slot01);
	UGP_SkillData* Slot02Skill = GetEquippedSkill(GPTags::Ability::Skill::Slot02);

	RebuildSkillEntries(AvailableSkills, Slot01Skill, Slot02Skill);
	RefreshSlotTexts(Slot01Skill, Slot02Skill);

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
	AGP_PlayerController* PlayerController = GetGPPlayerController();
	const bool bEquipped = PlayerController && PlayerController->RequestEquipSkill(SkillData, SlotTag);
	if (bEquipped)
	{
		RefreshSkillSelection();
	}
	return bEquipped;
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
	EquipSkillToSelectedSlot(SkillData);
}

void UGP_SkillSelectWidget::HandleSlot01Clicked()
{
	SelectSlot01();
}

void UGP_SkillSelectWidget::HandleSlot02Clicked()
{
	SelectSlot02();
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

void UGP_SkillSelectWidget::RebuildSkillEntries(const TArray<UGP_SkillData*>& AvailableSkills, UGP_SkillData* Slot01Skill, UGP_SkillData* Slot02Skill)
{
	if (!VerticalBox_SkillList || !SkillEntryWidgetClass)
	{
		return;
	}

	VerticalBox_SkillList->ClearChildren();
	SkillEntryWidgets.Reset();

	for (UGP_SkillData* SkillData : AvailableSkills)
	{
		if (!IsValid(SkillData))
		{
			continue;
		}

		UGP_SkillEntryWidget* EntryWidget = CreateWidget<UGP_SkillEntryWidget>(GetOwningPlayer(), SkillEntryWidgetClass);
		if (!IsValid(EntryWidget))
		{
			continue;
		}

		EntryWidget->SetupSkillEntry(SkillData);
		EntryWidget->SetEquipped(SkillData == Slot01Skill || SkillData == Slot02Skill);
		EntryWidget->OnSkillEntryClicked.RemoveDynamic(this, &ThisClass::HandleSkillEntryClicked);
		EntryWidget->OnSkillEntryClicked.AddDynamic(this, &ThisClass::HandleSkillEntryClicked);

		VerticalBox_SkillList->AddChildToVerticalBox(EntryWidget);
		SkillEntryWidgets.Add(EntryWidget);
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
		return FText::Format(NSLOCTEXT("SkillSelection", "EmptySlotFormat", "{0} : Empty"), FText::FromString(SlotLabel));
	}

	return FText::Format(NSLOCTEXT("SkillSelection", "EquippedSlotFormat", "{0} : {1}"), FText::FromString(SlotLabel), SkillData->SkillName);
}
