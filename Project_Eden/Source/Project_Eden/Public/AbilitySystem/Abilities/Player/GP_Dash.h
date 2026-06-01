#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_GameplayAbility.h"
#include "TimerManager.h"
#include "GP_Dash.generated.h"

class UAnimMontage;

/**
 * UGP_Dash
 *
 * 캐릭터의 대시(회피) 이동 어빌리티
 */
UCLASS()
class PROJECT_EDEN_API UGP_Dash : public UGP_GameplayAbility
{
	GENERATED_BODY()

public:
	// =========================================================================
	// 1. 라이프사이클 및 기본 오버라이드 함수군 (Lifecycle & Overrides)
	// =========================================================================
	UGP_Dash();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	// =========================================================================
	// 2. 액션 및 무브먼트 이벤트 콜백 함수군 (Action & Montage Callbacks)
	// =========================================================================
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION()
	void OnDashActionEnd(FGameplayEventData Payload);

	UFUNCTION()
	void OnFallbackActionEnd();

	void OnActionRootMotionCancelInput();

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

private:
	// =========================================================================
	// 3. 내부 상태 및 타이머 변수군 (Internal Status & Timer Handles)
	// =========================================================================
	FTimerHandle FallbackMontageEndTimerHandle;
	FTimerHandle FallbackActionEndTimerHandle;
	FDelegateHandle ActionRootMotionCancelInputHandle;

	TObjectPtr<UAnimMontage> ActiveDashMontage;
};
