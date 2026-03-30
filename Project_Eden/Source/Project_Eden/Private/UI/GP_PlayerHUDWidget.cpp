#include "UI/GP_PlayerHUDWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/GP_AttributeSet.h"
#include "Characters/GP_BaseCharacter.h"

UGP_PlayerHUDWidget::UGP_PlayerHUDWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	LocationText = FText::FromString(TEXT("LIMINAL ASHEN FIELD"));
	BossText = FText::FromString(TEXT("Omen of the Drowned Belfry"));
}

void UGP_PlayerHUDWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	RefreshPreview();
	if (AGP_BaseCharacter* BaseChar = Cast<AGP_BaseCharacter>(GetOwningPlayerPawn()))
	{
		if (UAbilitySystemComponent* ASC = BaseChar->GetAbilitySystemComponent())
		{
			BindToASC(ASC);
		}
		else
		{
			BaseChar->OnASCInitialized.AddDynamic(this, &ThisClass::OnASCInitializedCallback);
		}
	}
}

void UGP_PlayerHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshPreview();
}

void UGP_PlayerHUDWidget::BindToASC(UAbilitySystemComponent* InASC)
{
	if (!IsValid(InASC)) return;

	UGP_AttributeSet* AS = const_cast<UGP_AttributeSet*>(Cast<UGP_AttributeSet>(InASC->GetAttributeSet(UGP_AttributeSet::StaticClass())));
	if (!IsValid(AS)) return;

	auto BindWidgetDelegates = [InASC, AS](UGP_AttributeWidget* Widget)
	{
		if (!Widget) return;

		TTuple<FGameplayAttribute, FGameplayAttribute> Pair(Widget->Attribute, Widget->MaxAttribute);
        
		// 1. 바인딩 즉시 현재 수치로 1회 강제 업데이트
		Widget->OnAttributeChange(Pair, AS);

		// 메모리 보호를 위해 약은 포인터 생성
		TWeakObjectPtr<UGP_AttributeWidget> WeakWidget(Widget);
		TWeakObjectPtr<UGP_AttributeSet> WeakAS(AS);

		// 2. 현재값 변화 감지
		InASC->GetGameplayAttributeValueChangeDelegate(Pair.Key).AddLambda([WeakWidget, Pair, WeakAS](const FOnAttributeChangeData& Data)
		{
			if (WeakWidget.IsValid() && WeakAS.IsValid())
			{
				WeakWidget->OnAttributeChange(Pair, WeakAS.Get());
			}
		});

		// 3. 최대값 변화 감지
		InASC->GetGameplayAttributeValueChangeDelegate(Pair.Value).AddLambda([WeakWidget, Pair, WeakAS](const FOnAttributeChangeData& Data)
		{
			if (WeakWidget.IsValid() && WeakAS.IsValid())
			{
				WeakWidget->OnAttributeChange(Pair, WeakAS.Get());
			}
		});
	};

	BindWidgetDelegates(HealthBar);
	BindWidgetDelegates(ManaBar);
	BindWidgetDelegates(StaminaBar);
}

void UGP_PlayerHUDWidget::SetLocationText(const FText& InLocationText)
{
	LocationText = InLocationText;
	RefreshPreview();
}

void UGP_PlayerHUDWidget::SetBossText(const FText& InBossText)
{
	BossText = InBossText;
	RefreshPreview();
}

void UGP_PlayerHUDWidget::SetBossVisible(bool bIsVisible)
{
	bShowBossFrame = bIsVisible;
	RefreshPreview();
}

void UGP_PlayerHUDWidget::RefreshPreview()
{
	if (LocationTextBlock) LocationTextBlock->SetText(LocationText);
	if (BossTextBlock) BossTextBlock->SetText(BossText);
	if (BossFrame) BossFrame->SetVisibility(bShowBossFrame ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}

void UGP_PlayerHUDWidget::OnASCInitializedCallback(UAbilitySystemComponent* ASC, UAttributeSet* AS)
{
	BindToASC(ASC);
}
