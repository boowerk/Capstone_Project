#include "UI/GP_AttributeWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/ProgressBar.h"

void UGP_AttributeWidget::OnAttributeChange(const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair, UGP_AttributeSet* AttributeSet)
{
	const float AttributeValue = Pair.Key.GetNumericValue(AttributeSet);
	const float MaxAttributeValue = Pair.Value.GetNumericValue(AttributeSet);

	ApplyNativeProgressBarValue(AttributeValue, MaxAttributeValue);

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

void UGP_AttributeWidget::ApplyNativeProgressBarValue(float NewValue, float NewMaxValue)
{
	UProgressBar* ProgressBar = ResolveNativeProgressBar();
	if (!IsValid(ProgressBar))
	{
		return;
	}

	const float Percent = NewMaxValue > KINDA_SMALL_NUMBER
		? FMath::Clamp(NewValue / NewMaxValue, 0.0f, 1.0f)
		: 0.0f;

	ProgressBar->SetPercent(Percent);

	if (ShouldUseBossFillColorFallback())
	{
		// WBP_BossBar는 BP 그래프 없이 GP_AttributeWidget만 상속해도 보스 체력 색이 빨간색으로 보이게 합니다.
		ProgressBar->SetFillColorAndOpacity(FLinearColor(0.95f, 0.02f, 0.02f, 1.0f));
	}
}

UProgressBar* UGP_AttributeWidget::ResolveNativeProgressBar()
{
	if (IsValid(CachedNativeProgressBar))
	{
		return CachedNativeProgressBar;
	}

	if (!WidgetTree)
	{
		return nullptr;
	}

	static const FName CandidateNames[] =
	{
		TEXT("HealthBar"),
		TEXT("BossBar"),
		TEXT("ProgressBar"),
		TEXT("AttributeBar"),
		TEXT("Bar")
	};

	for (const FName& CandidateName : CandidateNames)
	{
		if (UProgressBar* CandidateProgressBar = Cast<UProgressBar>(GetWidgetFromName(CandidateName)))
		{
			CachedNativeProgressBar = CandidateProgressBar;
			return CachedNativeProgressBar;
		}
	}

	WidgetTree->ForEachWidget([this](UWidget* ChildWidget)
	{
		if (!IsValid(CachedNativeProgressBar))
		{
			CachedNativeProgressBar = Cast<UProgressBar>(ChildWidget);
		}
	});

	return CachedNativeProgressBar;
}

bool UGP_AttributeWidget::ShouldUseBossFillColorFallback() const
{
	// Boss 전용 WBP_BossBar처럼 이름에 Boss가 들어간 AttributeWidget만 기본 빨간 체력 색을 보정합니다.
	return GetName().Contains(TEXT("Boss")) || GetClass()->GetName().Contains(TEXT("Boss"));
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
