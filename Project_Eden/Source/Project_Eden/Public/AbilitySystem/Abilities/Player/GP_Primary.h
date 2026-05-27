// GP_Primary.h 수정본
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_SkillBase.h"
#include "GP_Primary.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;

UCLASS()
class PROJECT_EDEN_API UGP_Primary : public UGP_SkillBase
{
	GENERATED_BODY()

public:
	UGP_Primary();

    // ActivateAbility와 함께 InputPressed 오버라이드 추가
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    virtual void InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;

protected:
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	/** Primary 특유의 수직 오프셋 (부모 클래스 확장 시 통합 고려) */
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Skill|Values")
	float HitBoxElevationOffset = 20.0f;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Skill|Combo")
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
	TObjectPtr<UAbilityTask_WaitGameplayEvent> WaitLegacyPrimaryHitTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> WaitComboTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> WaitEndTask;

	void StartComboSequence();
	int32 GetNextComboIndex(int32 MaxComboCount);
	void ClearExistingTasks();

	// Some migrated primary montages still send the ability tag as their hit notify, so each swing guards against duplicate hit events.
	bool bHasAppliedCurrentAttackHit = false;

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
