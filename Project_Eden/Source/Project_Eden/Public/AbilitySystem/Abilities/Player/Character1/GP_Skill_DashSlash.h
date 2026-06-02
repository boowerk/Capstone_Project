#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_SkillBase.h"
#include "GameplayTagContainer.h"
#include "TimerManager.h"
#include "GP_Skill_DashSlash.generated.h"

class AActor;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;

/**
 * UGP_Skill_DashSlash
 *
 * 대시 슬래시 (돌진 베기) 공격 스킬 어빌리티
 */
UCLASS()
class PROJECT_EDEN_API UGP_Skill_DashSlash : public UGP_SkillBase
{
	GENERATED_BODY()

public:
	// =========================================================================
	// 1. 라이프사이클 및 기본 오버라이드 함수군 (Lifecycle & Overrides)
	// =========================================================================
	UGP_Skill_DashSlash();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual bool CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	// =========================================================================
	// 2. 어빌리티 종료 및 설정 변수군 (Tuning & Settings)
	// =========================================================================
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|DashSlash|Visuals")
	TSubclassOf<AActor> SlashVisualActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|DashSlash", meta = (ClampMin = "0.0"))
	float HitRange = 520.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|DashSlash", meta = (ClampMin = "0.0"))
	float HitWidth = 220.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|DashSlash", meta = (ClampMin = "0.0"))
	float HitHeight = 180.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|DashSlash")
	float HitHeightOffset = 80.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|DashSlash")
	float VisualForwardOffset = 260.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|DashSlash")
	float VisualHeightOffset = 90.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|DashSlash")
	FGameplayTag HitEventTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|DashSlash|Cooldown", meta = (ClampMin = "0.0"))
	float DefaultCooldownDuration = 5.0f;

private:
	// =========================================================================
	// 3. 내부 로직 및 헬퍼 함수군 (Skill Action Helpers)
	// =========================================================================
	void PerformDashSlashHit();
	void UnlockMovementControl();
	void ClearExistingTasks();

	// =========================================================================
	// 4. 애니메이션 몽타주 및 게임플레이 이벤트 콜백 (Gameplay Event Callbacks)
	// =========================================================================
	UFUNCTION()
	void OnAttackHitEventReceived(FGameplayEventData Payload);

	UFUNCTION()
	void OnActionEndEventReceived(FGameplayEventData Payload);

	UFUNCTION()
	void OnFallbackActionEnd();

	void OnActionRootMotionCancelInput();

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageBlendOut();

	UFUNCTION()
	void OnMontageInterrupted();

	// =========================================================================
	// 5. 내부 상태 및 타이머 변수군 (Internal Status & Timer Handles)
	// =========================================================================
	FTimerHandle FallbackMontageEndTimerHandle;
	FTimerHandle FallbackAttackHitTimerHandle;
	FTimerHandle FallbackActionEndTimerHandle;
	FDelegateHandle ActionRootMotionCancelInputHandle;

	bool bHasAppliedAttackHit = false;
	bool bMovementControlUnlocked = false;

	// =========================================================================
	// 6. GAS 어빌리티 태스크 변수군 (Gameplay Ability Tasks)
	// =========================================================================
	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> WaitHitTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> WaitEndTask;
};
