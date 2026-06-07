#include "UI/GP_PlayerHUDWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/GP_AttributeSet.h"
#include "Blueprint/WidgetTree.h"
#include "Characters/GP_BaseCharacter.h"
#include "GameFramework/Pawn.h"

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
	UpdateMinimapPlayerArrowRotation();

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

void UGP_PlayerHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	UpdateMinimapPlayerArrowRotation();
}

void UGP_PlayerHUDWidget::UpdateMinimapPlayerArrowRotation()
{
	if (!bRotateMinimapPlayerArrow)
	{
		return;
	}

	APawn* OwningPawn = GetOwningPlayerPawn();
	UWidget* ArrowWidget = ResolveMinimapPlayerArrowWidget();
	if (!IsValid(OwningPawn) || !IsValid(ArrowWidget))
	{
		bHasCachedMinimapPlayerArrowAngle = false;
		return;
	}

	const float RotationSign = bInvertMinimapPlayerArrowRotation ? -1.0f : 1.0f;
	const float DesiredAngle = FRotator::NormalizeAxis((OwningPawn->GetActorRotation().Yaw * RotationSign) + MinimapPlayerArrowAngleOffset);

	// 같은 각도를 반복해서 쓰지 않도록 캐시해 Slate transform 갱신 비용을 줄입니다.
	if (bHasCachedMinimapPlayerArrowAngle && FMath::IsNearlyEqual(CachedMinimapPlayerArrowAngle, DesiredAngle, 0.1f))
	{
		return;
	}

	ArrowWidget->SetRenderTransformAngle(DesiredAngle);
	CachedMinimapPlayerArrowAngle = DesiredAngle;
	bHasCachedMinimapPlayerArrowAngle = true;
}

UWidget* UGP_PlayerHUDWidget::ResolveMinimapPlayerArrowWidget() const
{
	if (IsValid(MinimapPlayerArrow))
	{
		return MinimapPlayerArrow.Get();
	}

	static const FName CandidateNames[] =
	{
		TEXT("MinimapPlayerArrow"),
		TEXT("MiniMapPlayerArrow"),
		TEXT("PlayerArrow"),
		TEXT("PlayerDirectionArrow"),
		TEXT("MapPlayerArrow")
	};

	for (const FName& CandidateName : CandidateNames)
	{
		if (UWidget* CandidateWidget = GetWidgetFromName(CandidateName))
		{
			return CandidateWidget;
		}
	}

	return nullptr;
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
		EnsureBossHealthAttributes(BossAttributeWidget);

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

void UGP_PlayerHUDWidget::BindAttributeWidgetToASC(UAbilitySystemComponent* InASC, UGP_AttributeWidget* Widget, UGP_AttributeSet* AttributeSet, TArray<FGPAttributeDelegateBinding>& DelegateHandles)
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
	DelegateHandles.Emplace(Pair.Key, CurrentValueHandle);

	FDelegateHandle MaxValueHandle = InASC->GetGameplayAttributeValueChangeDelegate(Pair.Value).AddLambda([WeakWidget, Pair, WeakAS](const FOnAttributeChangeData& Data)
	{
		if (WeakWidget.IsValid() && WeakAS.IsValid())
		{
			WeakWidget->OnAttributeChange(Pair, WeakAS.Get());
		}
	});
	DelegateHandles.Emplace(Pair.Value, MaxValueHandle);
}

void UGP_PlayerHUDWidget::RemoveAttributeDelegateHandles(TWeakObjectPtr<UAbilitySystemComponent>& BoundASC, TArray<FGPAttributeDelegateBinding>& DelegateHandles)
{
	if (BoundASC.IsValid())
	{
		for (const FGPAttributeDelegateBinding& Binding : DelegateHandles)
		{
			if (Binding.Attribute.IsValid() && Binding.Handle.IsValid())
			{
				// Remove the handle from the exact GAS attribute delegate it was added to.
				BoundASC->GetGameplayAttributeValueChangeDelegate(Binding.Attribute).Remove(Binding.Handle);
			}
		}
	}

	DelegateHandles.Reset();
	BoundASC.Reset();
}

void UGP_PlayerHUDWidget::EnsureBossHealthAttributes(UGP_AttributeWidget* Widget) const
{
	if (!IsValid(Widget))
	{
		return;
	}

	// WBP_BossBar is a GP_AttributeWidget, so force it to boss Health/MaxHealth even if the BP defaults are missing.
	Widget->Attribute = UGP_AttributeSet::GetHealthAttribute();
	Widget->MaxAttribute = UGP_AttributeSet::GetMaxHealthAttribute();
}

UGP_AttributeWidget* UGP_PlayerHUDWidget::ResolveAttributeWidgetFromWidget(UWidget* WidgetObject) const
{
	if (!IsValid(WidgetObject))
	{
		return nullptr;
	}

	if (UGP_AttributeWidget* AttributeWidget = Cast<UGP_AttributeWidget>(WidgetObject))
	{
		return AttributeWidget;
	}

	const UUserWidget* UserWidget = Cast<UUserWidget>(WidgetObject);
	if (!IsValid(UserWidget) || !UserWidget->WidgetTree)
	{
		return nullptr;
	}

	UGP_AttributeWidget* FoundWidget = nullptr;
	UserWidget->WidgetTree->ForEachWidget([&FoundWidget](UWidget* ChildWidget)
	{
		if (!FoundWidget)
		{
			FoundWidget = Cast<UGP_AttributeWidget>(ChildWidget);
		}
	});

	return FoundWidget;
}

UGP_AttributeWidget* UGP_PlayerHUDWidget::ResolveAttributeWidgetByName(const FName WidgetName) const
{
	// Boss bars may be either a direct GP_AttributeWidget or a wrapper UserWidget that contains one.
	return ResolveAttributeWidgetFromWidget(GetWidgetFromName(WidgetName));
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
		TEXT("WBP_BossBar"),
		TEXT("BossHealth"),
		TEXT("BossHPBar")
	};

	for (const FName& CandidateName : CandidateNames)
	{
		if (UGP_AttributeWidget* CandidateWidget = ResolveAttributeWidgetByName(CandidateName))
		{
			return CandidateWidget;
		}
	}

	if (UGP_AttributeWidget* CandidateWidget = ResolveAttributeWidgetFromWidget(BossFrame))
	{
		return CandidateWidget;
	}

	if (UGP_AttributeWidget* CandidateWidget = ResolveAttributeWidgetByName(TEXT("BossBox")))
	{
		return CandidateWidget;
	}

	UGP_AttributeWidget* FoundBossWidget = nullptr;
	WidgetTree->ForEachWidget([this, &FoundBossWidget](UWidget* ChildWidget)
	{
		if (!FoundBossWidget && ChildWidget && ChildWidget->GetName().Contains(TEXT("Boss")))
		{
			// As a last resort, scan only Boss-named widgets to avoid binding the player health bar by mistake.
			FoundBossWidget = ResolveAttributeWidgetFromWidget(ChildWidget);
		}
	});

	if (FoundBossWidget)
	{
		return FoundBossWidget;
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
