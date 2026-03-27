#include "UI/GP_WidgetComponent.h"

#include "AbilitySystem/GP_AbilitySystemComponent.h"
#include "AbilitySystem/GP_AttributeSet.h"
#include "Characters/GP_BaseCharacter.h"


void UGP_WidgetComponent::BeginPlay()
{
	Super::BeginPlay();

	InitAbilitySystemData();

	if (!IsASCInitialized())
	{
		GASCharacter->OnASCInitialized.AddDynamic(this, &ThisClass::OnASCInitialized);
	}
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

void UGP_WidgetComponent::OnASCInitialized(UAbilitySystemComponent* ASC, UAttributeSet* AS)
{
	AbilitySystemComponent = Cast<UGP_AbilitySystemComponent>(ASC);
	AttributeSet = Cast<UGP_AttributeSet>(AS);
	
	// TODO: Check if the Attribute Set has been initialized with the first GE
	// If not, bind to some delegate that will be broadcast when it is initialized.
	// 이 문제 해결에는 어트리뷰트 변경 감지, GE 적용 델리게이트, 초기화 태그 세가지정도 고려할것
}
