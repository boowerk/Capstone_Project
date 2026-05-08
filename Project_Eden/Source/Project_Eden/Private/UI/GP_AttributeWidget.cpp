#include "UI/GP_AttributeWidget.h"

void UGP_AttributeWidget::OnAttributeChange(const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair, UGP_AttributeSet* AttributeSet)
{
	const float AttributeValue = Pair.Key.GetNumericValue(AttributeSet);
	const float MaxAttributeValue = Pair.Value.GetNumericValue(AttributeSet);

	if (bHideWhenFull)
	{
		if (AttributeValue >= MaxAttributeValue || MaxAttributeValue <= 0.0f)
		{
			SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
	}

	BP_OnAttributeChange(AttributeValue, MaxAttributeValue);
}


bool UGP_AttributeWidget::MatchesAttributes(const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair) const
{
	return Attribute == Pair.Key && MaxAttribute == Pair.Value;
}

void UGP_AttributeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (bHideWhenFull)
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
}
