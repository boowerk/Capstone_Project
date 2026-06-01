#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_GameplayAbility.h"
#include "GP_Sprint.generated.h"

/**
 * UGP_Sprint
 *
 * 캐릭터의 질주(스프린트) 어빌리티
 */
UCLASS()
class PROJECT_EDEN_API UGP_Sprint : public UGP_GameplayAbility
{
	GENERATED_BODY()

public:
	// =========================================================================
	// 1. 기본 오버라이드 함수군 (Base Overrides)
	// =========================================================================
	UGP_Sprint();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
};
