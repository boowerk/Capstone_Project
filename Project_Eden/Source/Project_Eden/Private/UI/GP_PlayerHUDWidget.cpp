#include "UI/GP_PlayerHUDWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/GP_AttributeSet.h"
#include "Blueprint/WidgetTree.h"
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
}

void UGP_PlayerHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshPreview();

	// 1. 즉시 시도
	APawn* OwningPawn = GetOwningPlayerPawn();
	if (AGP_BaseCharacter* BaseChar = Cast<AGP_BaseCharacter>(OwningPawn))
	{
		if (UAbilitySystemComponent* ASC = BaseChar->GetAbilitySystemComponent())
		{
			BindToASC(ASC);
		}
		BaseChar->OnASCInitialized.RemoveAll(this);
		BaseChar->OnASCInitialized.AddDynamic(this, &ThisClass::OnASCInitializedCallback);
	}
	else
	{
		// 2. 아직 Pawn이 없다면, 타이머로 잠시 후 다시 시도 (클라이언트 초기화 지연 대응)
		GetWorld()->GetTimerManager().SetTimerForNextTick([this]() {
			NativeConstruct();
		});
	}
}

void UGP_PlayerHUDWidget::BindToASC(UAbilitySystemComponent* InASC)
{
	if (!IsValid(InASC)) return;

	UGP_AttributeSet* AS = const_cast<UGP_AttributeSet*>(Cast<UGP_AttributeSet>(InASC->GetAttributeSet(UGP_AttributeSet::StaticClass())));
	if (!IsValid(AS)) return;

	RemoveAttributeDelegateHandles(BoundPlayerASC, PlayerAttributeDelegateHandles);
	BoundPlayerASC = InASC;

	auto ResolveAttributeWidget = [this](UGP_AttributeWidget* BoundWidget, const FName WidgetName)
	{
		if (IsValid(BoundWidget))
		{
			return BoundWidget;
		}

		return Cast<UGP_AttributeWidget>(GetWidgetFromName(WidgetName));
	};

	BindAttributeWidgetToASC(InASC, ResolveAttributeWidget(HealthBar, TEXT("HealthBar")), AS, PlayerAttributeDelegateHandles);
	BindAttributeWidgetToASC(InASC, ResolveAttributeWidget(ManaBar, TEXT("ManaBar")), AS, PlayerAttributeDelegateHandles);
	BindAttributeWidgetToASC(InASC, ResolveAttributeWidget(StaminaBar, TEXT("StaminaBar")), AS, PlayerAttributeDelegateHandles);
}

void UGP_PlayerHUDWidget::BindBossToASC(UAbilitySystemComponent* InASC)
{
	if (!IsValid(InASC))
	{
		ClearBossASC();
		return;
	}

	UGP_AttributeSet* AS = const_cast<UGP_AttributeSet*>(Cast<UGP_AttributeSet>(InASC->GetAttributeSet(UGP_AttributeSet::StaticClass())));
	if (!IsValid(AS))
	{
		ClearBossASC();
		return;
	}

	if (UGP_AttributeWidget* BossAttributeWidget = ResolveBossHealthBar())
	{
		if (BoundBossASC == InASC && BossAttributeDelegateHandles.Num() > 0)
		{
			TTuple<FGameplayAttribute, FGameplayAttribute> Pair(BossAttributeWidget->Attribute, BossAttributeWidget->MaxAttribute);
			if (Pair.Key.IsValid() && Pair.Value.IsValid())
			{
				BossAttributeWidget->OnAttributeChange(Pair, AS);
			}
			return;
		}

		RemoveAttributeDelegateHandles(BoundBossASC, BossAttributeDelegateHandles);
		BoundBossASC = InASC;
		BindAttributeWidgetToASC(InASC, BossAttributeWidget, AS, BossAttributeDelegateHandles);
		return;
	}

	ClearBossASC();
}

void UGP_PlayerHUDWidget::ClearBossASC()
{
	RemoveAttributeDelegateHandles(BoundBossASC, BossAttributeDelegateHandles);
}

void UGP_PlayerHUDWidget::BindAttributeWidgetToASC(UAbilitySystemComponent* InASC, UGP_AttributeWidget* Widget, UGP_AttributeSet* AttributeSet, TArray<FDelegateHandle>& DelegateHandles)
{
	if (!IsValid(InASC) || !IsValid(Widget) || !IsValid(AttributeSet))
	{
		return;
	}

	TTuple<FGameplayAttribute, FGameplayAttribute> Pair(Widget->Attribute, Widget->MaxAttribute);
	if (!Pair.Key.IsValid() || !Pair.Value.IsValid())
	{
		return;
	}

	Widget->OnAttributeChange(Pair, AttributeSet);

	TWeakObjectPtr<UGP_AttributeWidget> WeakWidget(Widget);
	TWeakObjectPtr<UGP_AttributeSet> WeakAS(AttributeSet);

	FDelegateHandle CurrentValueHandle = InASC->GetGameplayAttributeValueChangeDelegate(Pair.Key).AddLambda([WeakWidget, Pair, WeakAS](const FOnAttributeChangeData& Data)
	{
		if (WeakWidget.IsValid() && WeakAS.IsValid())
		{
			WeakWidget->OnAttributeChange(Pair, WeakAS.Get());
		}
	});
	DelegateHandles.Add(CurrentValueHandle);

	FDelegateHandle MaxValueHandle = InASC->GetGameplayAttributeValueChangeDelegate(Pair.Value).AddLambda([WeakWidget, Pair, WeakAS](const FOnAttributeChangeData& Data)
	{
		if (WeakWidget.IsValid() && WeakAS.IsValid())
		{
			WeakWidget->OnAttributeChange(Pair, WeakAS.Get());
		}
	});
	DelegateHandles.Add(MaxValueHandle);
}

void UGP_PlayerHUDWidget::RemoveAttributeDelegateHandles(TWeakObjectPtr<UAbilitySystemComponent>& BoundASC, TArray<FDelegateHandle>& DelegateHandles)
{
	if (BoundASC.IsValid())
	{
		for (const FDelegateHandle& Handle : DelegateHandles)
		{
			BoundASC->GetGameplayAttributeValueChangeDelegate(UGP_AttributeSet::GetHealthAttribute()).Remove(Handle);
			BoundASC->GetGameplayAttributeValueChangeDelegate(UGP_AttributeSet::GetMaxHealthAttribute()).Remove(Handle);
			BoundASC->GetGameplayAttributeValueChangeDelegate(UGP_AttributeSet::GetManaAttribute()).Remove(Handle);
			BoundASC->GetGameplayAttributeValueChangeDelegate(UGP_AttributeSet::GetMaxManaAttribute()).Remove(Handle);
		}
	}

	DelegateHandles.Reset();
	BoundASC.Reset();
}

UGP_AttributeWidget* UGP_PlayerHUDWidget::ResolveBossHealthBar() const
{
	if (BossHealthBar)
	{
		return BossHealthBar;
	}

	if (!WidgetTree)
	{
		return nullptr;
	}

	static const FName CandidateNames[] =
	{
		TEXT("BossHealthBar"),
		TEXT("BossBar"),
		TEXT("BossHealth"),
		TEXT("BossHPBar")
	};

	for (const FName& CandidateName : CandidateNames)
	{
		if (UGP_AttributeWidget* CandidateWidget = Cast<UGP_AttributeWidget>(GetWidgetFromName(CandidateName)))
		{
			return CandidateWidget;
		}
	}

	return nullptr;
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
