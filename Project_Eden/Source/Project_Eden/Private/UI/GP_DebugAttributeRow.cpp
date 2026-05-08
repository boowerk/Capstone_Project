#include "UI/GP_DebugAttributeRow.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"

void UGP_DebugAttributeRow::InitializeRow(const FString& InLabel, const FGameplayAttribute& InAttribute, UAbilitySystemComponent* InASC)
{
	if (BackgroundBorder)
	{
		// 반투명한 검은색 배경 설정
		BackgroundBorder->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.5f));
	}

	if (LabelText)
	{
		LabelText->SetText(FText::FromString(InLabel));
		// 텍스트 그림자 추가로 가독성 향상
		LabelText->SetShadowOffset(FVector2D(1.0f, 1.0f));
		LabelText->SetShadowColorAndOpacity(FLinearColor::Black);
	}

	TargetAttribute = InAttribute;

	if (InASC && InAttribute.IsValid())
	{
		// 초기값 설정
		RefreshValue();

		if (ValueText)
		{
			ValueText->SetShadowOffset(FVector2D(1.0f, 1.0f));
			ValueText->SetShadowColorAndOpacity(FLinearColor::Black);
		}

		// 값이 변할 때마다 업데이트하도록 델리게이트 바인딩
		InASC->GetGameplayAttributeValueChangeDelegate(InAttribute).AddUObject(this, &ThisClass::UpdateValue);
	}
}

void UGP_DebugAttributeRow::UpdateValue(const FOnAttributeChangeData& Data)
{
	RefreshValue();
}

void UGP_DebugAttributeRow::RefreshValue()
{
	if (!ValueText || !TargetAttribute.IsValid()) return;

	AActor* OwningActor = GetOwningPlayerPawn();
	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(OwningActor))
	{
		UAbilitySystemComponent* CurrentASC = ASI->GetAbilitySystemComponent();
		if (CurrentASC)
		{
			float Val = CurrentASC->GetNumericAttribute(TargetAttribute);
			ValueText->SetText(FText::AsNumber(Val));
			
			// 수치 변화 시 시각적 효과 (노란색으로 반짝임)
			ValueText->SetColorAndOpacity(FSlateColor(FLinearColor::Yellow));
			
			FTimerHandle TimerHandle;
			GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]() {
				if (ValueText) ValueText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			}, 0.5f, false);
		}
	}
}
