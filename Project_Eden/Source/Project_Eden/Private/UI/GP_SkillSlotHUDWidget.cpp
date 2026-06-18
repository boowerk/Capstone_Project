#include "UI/GP_SkillSlotHUDWidget.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GP_SkillData.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameplayEffectTypes.h"

void UGP_SkillSlotHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UGP_SkillSlotHUDWidget::SetupSlot(UAbilitySystemComponent* InASC, UGP_SkillData* InSkillData, const FText& InKeyLabel)
{
	BoundASC = InASC;
	SkillData = InSkillData;

	if (SkillData)
	{
		CooldownTag = SkillData->CooldownTag;
		CooldownDuration = SkillData->CooldownDuration;
	}
	else
	{
		CooldownTag = FGameplayTag();
		CooldownDuration = 0.f;
	}

	if (Text_Key)
	{
		Text_Key->SetText(InKeyLabel);
	}

	RefreshIcon();
	RefreshCooldown();
}

void UGP_SkillSlotHUDWidget::ClearSlot()
{
	BoundASC.Reset();
	SkillData = nullptr;
	CooldownTag = FGameplayTag();
	CooldownDuration = 0.f;

	if (Image_Icon)
	{
		Image_Icon->SetVisibility(ESlateVisibility::Hidden);
	}
	if (ProgressBar_Cooldown)
	{
		ProgressBar_Cooldown->SetPercent(0.f);
		ProgressBar_Cooldown->SetVisibility(ESlateVisibility::Hidden);
	}
	if (Text_Cooldown)
	{
		Text_Cooldown->SetText(FText::GetEmpty());
	}
}

void UGP_SkillSlotHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshCooldown();
}

void UGP_SkillSlotHUDWidget::RefreshIcon()
{
	if (!Image_Icon)
	{
		return;
	}

	if (!SkillData || SkillData->SkillIcon.IsNull())
	{
		Image_Icon->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	UTexture2D* IconTexture = SkillData->SkillIcon.LoadSynchronous();
	if (IsValid(IconTexture))
	{
		Image_Icon->SetBrushFromTexture(IconTexture);
		Image_Icon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	else
	{
		Image_Icon->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UGP_SkillSlotHUDWidget::RefreshCooldown()
{
	const bool bHasSkill = SkillData != nullptr;
	const bool bHasTag = CooldownTag.IsValid();
	const bool bHasASC = BoundASC.IsValid();

	if (!bHasSkill || !bHasTag || !bHasASC || CooldownDuration <= 0.f)
	{
		if (ProgressBar_Cooldown)
		{
			ProgressBar_Cooldown->SetPercent(0.f);
			ProgressBar_Cooldown->SetVisibility(ESlateVisibility::Hidden);
		}
		if (Text_Cooldown)
		{
			Text_Cooldown->SetText(FText::GetEmpty());
		}
		return;
	}

	// Query remaining cooldown from active gameplay effects that own the cooldown tag.
	FGameplayTagContainer Tags;
	Tags.AddTag(CooldownTag);
	const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(Tags);
	TArray<TPair<float, float>> Pairs = BoundASC->GetActiveEffectsTimeRemainingAndDuration(Query);

	float TimeRemaining = 0.f;
	if (Pairs.Num() > 0)
	{
		TimeRemaining = FMath::Max(0.f, Pairs[0].Key);
	}

	const bool bOnCooldown = TimeRemaining > 0.f;

	if (ProgressBar_Cooldown)
	{
		if (bOnCooldown)
		{
			ProgressBar_Cooldown->SetPercent(TimeRemaining / CooldownDuration);
			ProgressBar_Cooldown->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			ProgressBar_Cooldown->SetPercent(0.f);
			ProgressBar_Cooldown->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (Text_Cooldown)
	{
		if (bOnCooldown)
		{
			// Show one decimal place for <10s, whole seconds otherwise.
			const FString CooldownStr = TimeRemaining < 10.f
				? FString::Printf(TEXT("%.1f"), TimeRemaining)
				: FString::Printf(TEXT("%d"), FMath::CeilToInt(TimeRemaining));
			Text_Cooldown->SetText(FText::FromString(CooldownStr));
		}
		else
		{
			Text_Cooldown->SetText(FText::GetEmpty());
		}
	}
}
