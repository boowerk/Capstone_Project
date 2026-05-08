#include "UI/GP_WidgetComponent.h"

#include "AbilitySystem/GP_AbilitySystemComponent.h"
#include "AbilitySystem/GP_AttributeSet.h"
#include "Characters/GP_BaseCharacter.h"
#include "UI/GP_AttributeWidget.h"
#include "Blueprint/WidgetTree.h"
#include "GeometryCollection/GeometryCollectionParticlesData.h"

void UGP_WidgetComponent::BeginPlay()
{
	Super::BeginPlay();

	InitAbilitySystemData();

	if (!IsASCInitialized())
	{
		GASCharacter->OnASCInitialized.AddDynamic(this, &ThisClass::OnASCInitialized);
		return;
	}
	InitializeAttributeDelegate();
}

void UGP_WidgetComponent::InitAbilitySystemData()
{
	GASCharacter = Cast<AGP_BaseCharacter>(GetOwner());
	AttributeSet = Cast<UGP_AttributeSet>(GASCharacter->GetAttributeSet());
	AbilitySystemComponent = Cast<UGP_AbilitySystemComponent>(GASCharacter->GetAbilitySystemComponent());
}

bool UGP_WidgetComponent::IsASCInitialized() const
{
	return AbilitySystemComponent.IsValid() && AttributeSet.IsValid();
}

void UGP_WidgetComponent::InitializeAttributeDelegate()
{
	if (!AttributeSet->bAttributesInitialized)
	{
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
	InitializeAttributeDelegate();
}
void UGP_WidgetComponent::BindToAttributeChanges()
{
	
	UUserWidget* UserWidget = GetUserWidgetObject();
	if (!IsValid(UserWidget)) return;
	
	for (const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair : AttributeMap)
	{
		BindWidgetToAttributeChanges(GetUserWidgetObject(), Pair);
		if( UserWidget->WidgetTree)
		{
			UserWidget->WidgetTree->ForEachWidget([this,Pair](UWidget* ChildWidget)
			{
				BindWidgetToAttributeChanges(ChildWidget, Pair);
			});
		}
		GetUserWidgetObject()->WidgetTree->ForEachWidget([this, &Pair](UWidget* ChildWidget)
		{
			BindWidgetToAttributeChanges(ChildWidget, Pair);
		});
	}
}

void UGP_WidgetComponent::BindWidgetToAttributeChanges(UWidget* WidgetObject, const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair) const
{
	UGP_AttributeWidget* AttributeWidget = Cast<UGP_AttributeWidget>(WidgetObject);
	if (!IsValid(AttributeWidget)) return;
	if (!AttributeWidget->MatchesAttributes(Pair)) return; 

	AttributeWidget->OnAttributeChange(Pair, AttributeSet.Get());

	// 메모리 누수 및 크래시 방지를 위해 약은 포인터(TWeakObjectPtr)로 캡처
	TWeakObjectPtr<UGP_AttributeWidget> WeakWidget(AttributeWidget);
	TWeakObjectPtr<UGP_AttributeSet> WeakAS(AttributeSet.Get());

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Pair.Key).AddLambda([WeakWidget, Pair, WeakAS](const FOnAttributeChangeData& AttributeChangeData)
	{
		// 람다가 실행될 때 위젯과 AttributeSet이 유효한지 검사
		if (WeakWidget.IsValid() && WeakAS.IsValid())
		{
			WeakWidget->OnAttributeChange(Pair, WeakAS.Get());
		}
	});
}


