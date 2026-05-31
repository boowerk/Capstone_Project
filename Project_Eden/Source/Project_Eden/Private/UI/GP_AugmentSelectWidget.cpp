#include "UI/GP_AugmentSelectWidget.h"

#include "AbilitySystem/Abilities/GP_SkillAugmentData.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Player/GP_PlayerState.h"

void UGP_AugmentSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Augment0) { Button_Augment0->OnClicked.AddDynamic(this, &UGP_AugmentSelectWidget::SelectAugment0); }
	if (Button_Augment1) { Button_Augment1->OnClicked.AddDynamic(this, &UGP_AugmentSelectWidget::SelectAugment1); }
	if (Button_Augment2) { Button_Augment2->OnClicked.AddDynamic(this, &UGP_AugmentSelectWidget::SelectAugment2); }

	RefreshCandidateTexts();
}

void UGP_AugmentSelectWidget::SetCandidateAugments(const TArray<UGP_SkillAugmentData*>& NewCandidateAugments)
{
	CandidateAugments.Reset();
	CandidateAugments.Reserve(NewCandidateAugments.Num());

	for (UGP_SkillAugmentData* AugmentData : NewCandidateAugments)
	{
		CandidateAugments.Add(AugmentData);
	}

	RefreshCandidateTexts();
}

bool UGP_AugmentSelectWidget::SelectCandidateIndex(int32 CandidateIndex)
{
	UGP_SkillAugmentData* AugmentData = GetCandidateAugment(CandidateIndex);
	if (!IsValid(AugmentData))
	{
		return false;
	}

	AGP_PlayerState* PlayerState = GetGPPlayerState();
	if (!IsValid(PlayerState))
	{
		return false;
	}

	PlayerState->AddSkillAugment(AugmentData);
	OnAugmentSelected(AugmentData);

	if (bRemoveOnSelection)
	{
		RemoveFromParent();
	}

	return true;
}

UGP_SkillAugmentData* UGP_AugmentSelectWidget::GetCandidateAugment(int32 CandidateIndex) const
{
	return CandidateAugments.IsValidIndex(CandidateIndex) ? CandidateAugments[CandidateIndex].Get() : nullptr;
}

void UGP_AugmentSelectWidget::SelectAugment0()
{
	SelectCandidateIndex(0);
}

void UGP_AugmentSelectWidget::SelectAugment1()
{
	SelectCandidateIndex(1);
}

void UGP_AugmentSelectWidget::SelectAugment2()
{
	SelectCandidateIndex(2);
}

void UGP_AugmentSelectWidget::RefreshCandidateTexts()
{
	if (TextBlock_Augment0) { TextBlock_Augment0->SetText(GetAugmentDisplayText(GetCandidateAugment(0))); }
	if (TextBlock_Augment1) { TextBlock_Augment1->SetText(GetAugmentDisplayText(GetCandidateAugment(1))); }
	if (TextBlock_Augment2) { TextBlock_Augment2->SetText(GetAugmentDisplayText(GetCandidateAugment(2))); }
}

FText UGP_AugmentSelectWidget::GetAugmentDisplayText(const UGP_SkillAugmentData* AugmentData) const
{
	if (!IsValid(AugmentData))
	{
		return FText::GetEmpty();
	}

	if (!AugmentData->AugmentName.IsEmpty())
	{
		return AugmentData->AugmentName;
	}

	return FText::FromString(AugmentData->GetName());
}

AGP_PlayerState* UGP_AugmentSelectWidget::GetGPPlayerState() const
{
	APlayerController* OwningPlayer = GetOwningPlayer();
	return IsValid(OwningPlayer) ? OwningPlayer->GetPlayerState<AGP_PlayerState>() : nullptr;
}
