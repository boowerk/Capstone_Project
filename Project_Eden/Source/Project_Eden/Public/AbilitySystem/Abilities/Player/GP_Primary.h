// GP_Primary.h 수정본
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_GameplayAbility.h"
#include "GP_Primary.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;

UCLASS()
class PROJECT_EDEN_API UGP_Primary : public UGP_GameplayAbility
{
	GENERATED_BODY()

public:
	UGP_Primary();
    
    // ActivateAbility와 함께 InputPressed 오버라이드 추가
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    virtual void InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Effects")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Abilities")
	float HitBoxRadius = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Abilities")
	float HitBoxForwardOffset = 200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Abilities")
	float HitBoxElevationOffset = 20.0f;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Combo")
	bool bUseRandomCombo = false;

private:
	int32 CurrentComboIndex = 0;
	bool bHasQueuedNextAttack = false;
	bool bIsComboWindowOpen = false;

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> WaitHitTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> WaitComboTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> WaitEndTask;

	void StartComboSequence();
	int32 GetNextComboIndex(int32 MaxComboCount);
	void ClearExistingTasks();

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnAttackHitEventReceived(FGameplayEventData Payload);

	UFUNCTION()
	void OnComboEnableEventReceived(FGameplayEventData Payload);

	UFUNCTION()
	void OnActionEndEventReceived(FGameplayEventData Payload);
};
