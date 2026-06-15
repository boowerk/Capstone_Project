#include "UI/GP_SkillSelectWidget.h"

#include "AbilitySystem/Abilities/GP_SkillData.h"
#include "AbilitySystem/Abilities/GP_SkillPoolData.h"
#include "GameplayTags/GP_Tags.h"
#include "Player/GP_PlayerController.h"
#include "Player/GP_PlayerState.h"

void UGP_SkillSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SelectedSlotTag = GPTags::Ability::Skill::Slot01;

	if (AGP_PlayerState* PlayerState = GetGPPlayerState())
	{
		PlayerState->OnEquippedSkillChanged.RemoveDynamic(this, &ThisClass::HandleEquippedSkillChanged);
		PlayerState->OnEquippedSkillChanged.AddDynamic(this, &ThisClass::HandleEquippedSkillChanged);
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
	BP_OnSkillSelectionRefreshed(
		GetAvailableSkills(),
		GetEquippedSkill(GPTags::Ability::Skill::Slot01),
		GetEquippedSkill(GPTags::Ability::Skill::Slot02),
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
	return PlayerController && PlayerController->RequestEquipSkill(SkillData, SlotTag);
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
