#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_GameplayAbility.h"
#include "GP_Ultimate.generated.h"

/**
 * UGP_Ultimate
 *
 * 캐릭터의 궁극기 어빌리티
 */
UCLASS()
class PROJECT_EDEN_API UGP_Ultimate : public UGP_GameplayAbility
{
	GENERATED_BODY()

public:
	// =========================================================================
	// 1. 기본 오버라이드 함수군 (Base Overrides)
	// =========================================================================
	UGP_Ultimate();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
