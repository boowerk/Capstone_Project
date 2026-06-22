#include "UI/GP_WidgetComponent.h"

#include "AbilitySystem/GP_AbilitySystemComponent.h"
#include "AbilitySystem/GP_AttributeSet.h"
#include "Characters/GP_BaseCharacter.h"
#include "UI/GP_AttributeWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Engine.h"
#include "GeometryCollection/GeometryCollectionParticlesData.h"

UGP_WidgetComponent::UGP_WidgetComponent()
{
	// Enemy world-space bars use the shared GAS Health contract unless a component adds more attribute pairs.
	AttributeMap.Add(UGP_AttributeSet::GetHealthAttribute(), UGP_AttributeSet::GetMaxHealthAttribute());
}

bool UGP_WidgetComponent::TracksAttributePair(const FGameplayAttribute& Attribute, const FGameplayAttribute& MaxAttribute) const
{
	const FGameplayAttribute* ConfiguredMaxAttribute = AttributeMap.Find(Attribute);
	return ConfiguredMaxAttribute != nullptr && *ConfiguredMaxAttribute == MaxAttribute;
}

void UGP_WidgetComponent::BeginPlay()
{
	Super::BeginPlay();

	InitAbilitySystemData();

	if (!GASCharacter.IsValid())
	{
		return;
	}

	if (!IsASCInitialized())
	{
		GASCharacter->OnASCInitialized.RemoveDynamic(this, &ThisClass::OnASCInitialized);
		GASCharacter->OnASCInitialized.AddDynamic(this, &ThisClass::OnASCInitialized);
		return;
	}
	InitializeAttributeDelegate();
}

void UGP_WidgetComponent::InitAbilitySystemData()
{
	GASCharacter = Cast<AGP_BaseCharacter>(GetOwner());
	if (!GASCharacter.IsValid())
	{
		AttributeSet.Reset();
		AbilitySystemComponent.Reset();
		return;
	}

	AttributeSet = Cast<UGP_AttributeSet>(GASCharacter->GetAttributeSet());
	AbilitySystemComponent = Cast<UGP_AbilitySystemComponent>(GASCharacter->GetAbilitySystemComponent());
}

bool UGP_WidgetComponent::IsASCInitialized() const
{
	return AbilitySystemComponent.IsValid() && AttributeSet.IsValid();
}

void UGP_WidgetComponent::InitializeAttributeDelegate()
{
	if (!IsASCInitialized() || bBoundAttributeChanges)
	{
		return;
	}

	if (!AttributeSet->bAttributesInitialized)
	{
		AttributeSet->OnAttributesInitialized.RemoveDynamic(this, &ThisClass::BindToAttributeChanges);
		AttributeSet->OnAttributesInitialized.AddDynamic(this, &ThisClass::BindToAttributeChanges);
	}
	else
	{
		BindToAttributeChanges();
	}
}

void UGP_WidgetComponent::OnASCInitialized(UAbilitySystemComponent* ASC, UAttributeSet* AS)
{
	AbilitySystemComponent = Cast<UGP_AbilitySystemComponent>(ASC);
	AttributeSet = Cast<UGP_AttributeSet>(AS);
	//전에 있던 유효성문제는 임시 델리게이트 구현으로 해결
	if (!IsASCInitialized()) return; 
	if (GASCharacter.IsValid())
	{
		GASCharacter->OnASCInitialized.RemoveDynamic(this, &ThisClass::OnASCInitialized);
	}
	InitializeAttributeDelegate();
}
void UGP_WidgetComponent::BindToAttributeChanges()
{
	if (bBoundAttributeChanges || !IsASCInitialized())
	{
		return;
	}
	
	UUserWidget* UserWidget = GetUserWidgetObject();
	if (!IsValid(UserWidget)) return;

	bool bAnyWidgetBound = false;
	
	for (const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair : AttributeMap)
	{
		bAnyWidgetBound |= BindWidgetToAttributeChanges(UserWidget, Pair);
		if (UserWidget->WidgetTree)
		{
			UserWidget->WidgetTree->ForEachWidget([this, Pair, &bAnyWidgetBound](UWidget* ChildWidget)
			{
				bAnyWidgetBound |= BindWidgetToAttributeChanges(ChildWidget, Pair);
			});
		}
	}

	bBoundAttributeChanges = bAnyWidgetBound;

	if (bDebugAttributeChanges && !bAnyWidgetBound)
	{
		const FString DebugMessage = FString::Printf(TEXT("[HP UI Debug] No attribute widgets bound for %s"), *GetNameSafe(GetOwner()));
		UE_LOG(LogTemp, Warning, TEXT("%s"), *DebugMessage);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, DebugAttributeMessageDuration, FColor::Yellow, DebugMessage);
		}
	}
}

void UGP_WidgetComponent::DebugLogAttributeUpdate(const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair, float NewValue, float NewMaxValue) const
{
	if (!bDebugAttributeChanges)
	{
		return;
	}

	const FString DebugMessage = FString::Printf(
		TEXT("[HP UI Debug] %s %s -> %.1f / %.1f"),
		*GetNameSafe(GetOwner()),
		*Pair.Key.GetName(),
		NewValue,
		NewMaxValue);

	UE_LOG(LogTemp, Log, TEXT("%s"), *DebugMessage);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, DebugAttributeMessageDuration, FColor::Cyan, DebugMessage);
	}
}

bool UGP_WidgetComponent::BindWidgetToAttributeChanges(UWidget* WidgetObject, const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair) const
{
	UGP_AttributeWidget* AttributeWidget = Cast<UGP_AttributeWidget>(WidgetObject);
	if (!IsValid(AttributeWidget)) return false;
	if (!AttributeWidget->MatchesAttributes(Pair)) return false;

	AttributeWidget->OnAttributeChange(Pair, AttributeSet.Get());
	DebugLogAttributeUpdate(Pair, Pair.Key.GetNumericValue(AttributeSet.Get()), Pair.Value.GetNumericValue(AttributeSet.Get()));

	// 메모리 누수 및 크래시 방지를 위해 약은 포인터(TWeakObjectPtr)로 캡처
	TWeakObjectPtr<UGP_AttributeWidget> WeakWidget(AttributeWidget);
	TWeakObjectPtr<UGP_AttributeSet> WeakAS(AttributeSet.Get());
	TWeakObjectPtr<UGP_WidgetComponent> WeakComponent(const_cast<UGP_WidgetComponent*>(this));

	const auto RefreshWidget = [WeakWidget, Pair, WeakAS, WeakComponent](const FOnAttributeChangeData& AttributeChangeData)
	{
		// 람다가 실행될 때 위젯과 AttributeSet이 유효한지 검사
		if (WeakWidget.IsValid() && WeakAS.IsValid())
		{
			WeakWidget->OnAttributeChange(Pair, WeakAS.Get());
			if (WeakComponent.IsValid())
			{
				WeakComponent->DebugLogAttributeUpdate(
					Pair,
					Pair.Key.GetNumericValue(WeakAS.Get()),
					Pair.Value.GetNumericValue(WeakAS.Get()));
			}
		}
	};

	// Current and max attributes both affect the displayed percent, so listen to both.
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Pair.Key).AddLambda(RefreshWidget);
	if (Pair.Value.IsValid())
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Pair.Value).AddLambda(RefreshWidget);
	}

	return true;
}


