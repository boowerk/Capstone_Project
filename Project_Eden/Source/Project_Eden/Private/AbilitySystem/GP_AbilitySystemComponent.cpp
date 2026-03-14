#include "AbilitySystem/GP_AbilitySystemComponent.h"

#include "GameplayTags/GP_Tags.h"


// 주의! 
// AbilitySpec은 CDO값으로 실제 인스턴스에서 실시간으로 얻은 값이 아닌 CDO 즉 템플릿 값이라
// 읽기 전용으로 태그 데이터만 읽어오게 구현했음 - 슝

void UGP_AbilitySystemComponent::OnGiveAbility(FGameplayAbilitySpec& AbilitySpec)
{
	Super::OnGiveAbility(AbilitySpec);
	
	HandleAutoActivatedAbility(AbilitySpec);
}

void UGP_AbilitySystemComponent::OnRep_ActivateAbilities()
{
	Super::OnRep_ActivateAbilities();
	
	FScopedAbilityListLock ActivateScopeLock(*this);
	for (const FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		HandleAutoActivatedAbility(AbilitySpec);
	}
}

void UGP_AbilitySystemComponent::HandleAutoActivatedAbility(const FGameplayAbilitySpec& AbilitySpec)
{
	if (!IsValid(AbilitySpec.Ability)) return;
	for (const FGameplayTag& Tag : AbilitySpec.Ability->GetAssetTags()) 
	{
		if (Tag.MatchesTagExact(GPTags::GPAbilities::ActivateOnGiven))
		{
			TryActivateAbility(AbilitySpec.Handle);
		} 
		
	}
}
