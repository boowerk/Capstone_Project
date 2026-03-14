#include "AbilitySystem/GP_AbilitySystemComponent.h"

#include "GameplayTags/GP_Tags.h"


// 주의! 
// AbilitySpec은 CDO값으로 실제 인스턴스에서 실시간으로 얻은 값이 아닌 CDO 즉 템플릿 값이라
// 읽기 전용으로 태그 데이터만 읽어오게 구현했음 - 슝

// 서버에서 ASC에 어빌리티 부여시 알아서 호출됨
void UGP_AbilitySystemComponent::OnGiveAbility(FGameplayAbilitySpec& AbilitySpec) 
{
	Super::OnGiveAbility(AbilitySpec);
	
	HandleAutoActivatedAbility(AbilitySpec);
}

// 클라에서 어빌리티 복제시 갱신되어 호출됨
void UGP_AbilitySystemComponent::OnRep_ActivateAbilities()
{
	Super::OnRep_ActivateAbilities();
	
	FScopedAbilityListLock ActivateScopeLock(*this);
	for (const FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		HandleAutoActivatedAbility(AbilitySpec);
	}
}

// 어빌리티의 태그를 검사하여 특정 태그가 있다면 실행시키는 로직
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
