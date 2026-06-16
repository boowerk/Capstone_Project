#include "UI/GP_AugmentSelectWidget.h"

#include "AbilitySystem/Abilities/GP_SkillAugmentData.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Player/GP_PlayerController.h"
#include "Player/GP_PlayerState.h"

UGP_AugmentSelectWidget::UGP_AugmentSelectWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AugmentTypeBackgrounds.Add(EGP_SkillAugmentType::BasicAttack, TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/Debug/UI/Augments/Icons/Dawn.Dawn"))));
	AugmentTypeBackgrounds.Add(EGP_SkillAugmentType::Skill, TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/Debug/UI/Augments/Icons/Dusk.Dusk"))));
	AugmentTypeBackgrounds.Add(EGP_SkillAugmentType::Ultimate, TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/Debug/UI/Augments/Icons/Midnight.Midnight"))));
	AugmentTypeBackgrounds.Add(EGP_SkillAugmentType::Passive, TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/Debug/UI/Augments/Icons/Zenith.Zenith"))));
}

void UGP_AugmentSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Augment0) { Button_Augment0->OnClicked.AddDynamic(this, &UGP_AugmentSelectWidget::SelectAugment0); }
	if (Button_Augment1) { Button_Augment1->OnClicked.AddDynamic(this, &UGP_AugmentSelectWidget::SelectAugment1); }
	if (Button_Augment2) { Button_Augment2->OnClicked.AddDynamic(this, &UGP_AugmentSelectWidget::SelectAugment2); }

	RefreshCandidateViews();
}

void UGP_AugmentSelectWidget::SetCandidateAugments(const TArray<UGP_SkillAugmentData*>& NewCandidateAugments)
{
	CandidateAugments.Reset();
	CandidateAugments.Reserve(NewCandidateAugments.Num());

	for (UGP_SkillAugmentData* AugmentData : NewCandidateAugments)
	{
		CandidateAugments.Add(AugmentData);
	}

	RefreshCandidateViews();
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
		if (AGP_PlayerController* PlayerController = Cast<AGP_PlayerController>(GetOwningPlayer()))
		{
			PlayerController->CloseAugmentSelectWidget();
		}
		else
		{
			RemoveFromParent();
		}
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

void UGP_AugmentSelectWidget::RefreshCandidateViews()
{
	RefreshCandidateView(0, TextBlock_Augment0, TextBlock_Description0, Image_Icon0, Image_CardBg0);
	RefreshCandidateView(1, TextBlock_Augment1, TextBlock_Description1, Image_Icon1, Image_CardBg1);
	RefreshCandidateView(2, TextBlock_Augment2, TextBlock_Description2, Image_Icon2, Image_CardBg2);
}

void UGP_AugmentSelectWidget::RefreshCandidateView(int32 CandidateIndex, UTextBlock* NameText, UTextBlock* DescriptionText, UImage* IconImage, UImage* BackgroundImage)
{
	const UGP_SkillAugmentData* AugmentData = GetCandidateAugment(CandidateIndex);

	if (NameText)
	{
		NameText->SetText(GetAugmentDisplayText(AugmentData));
	}

	if (DescriptionText)
	{
		DescriptionText->SetText(IsValid(AugmentData) ? AugmentData->AugmentDescription : FText::GetEmpty());
	}

	if (IconImage)
	{
		UTexture2D* IconTexture = IsValid(AugmentData) ? AugmentData->AugmentIcon.LoadSynchronous() : nullptr;
		IconImage->SetVisibility(IsValid(IconTexture) ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
		if (IsValid(IconTexture))
		{
			IconImage->SetBrushFromTexture(IconTexture, true);
		}
	}

	if (BackgroundImage)
	{
		UTexture2D* BackgroundTexture = GetAugmentTypeBackgroundTexture(AugmentData);
		BackgroundImage->SetVisibility(IsValid(BackgroundTexture) ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		if (IsValid(BackgroundTexture))
		{
			BackgroundImage->SetBrushFromTexture(BackgroundTexture, true);
		}
	}
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

UTexture2D* UGP_AugmentSelectWidget::GetAugmentTypeBackgroundTexture(const UGP_SkillAugmentData* AugmentData) const
{
	if (!IsValid(AugmentData))
	{
		return nullptr;
	}

	const TSoftObjectPtr<UTexture2D>* BackgroundTexture = AugmentTypeBackgrounds.Find(AugmentData->AugmentType);
	return BackgroundTexture ? BackgroundTexture->LoadSynchronous() : nullptr;
}

AGP_PlayerState* UGP_AugmentSelectWidget::GetGPPlayerState() const
{
	APlayerController* OwningPlayer = GetOwningPlayer();
	return IsValid(OwningPlayer) ? OwningPlayer->GetPlayerState<AGP_PlayerState>() : nullptr;
}
