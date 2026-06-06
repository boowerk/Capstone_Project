// GP_Primary.h 수정본
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_SkillBase.h"
#include "TimerManager.h"
#include "GP_Primary.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class UNiagaraComponent;
class UNiagaraSystem;
class UGP_SkillData;

/**
 * UGP_Primary
 *
 * 기본 공격(콤보 공격) 어빌리티
 */
UCLASS()
class PROJECT_EDEN_API UGP_Primary : public UGP_SkillBase
{
	GENERATED_BODY()

public:
	// =========================================================================
	// 1. 라이프사이클 및 기본 오버라이드 함수군 (Lifecycle & Overrides)
	// =========================================================================
	UGP_Primary();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;

protected:
	// =========================================================================
	// 2. 어빌리티 종료 및 설정 변수군 (Tuning & Settings)
	// =========================================================================
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** Primary 특유의 수직 오프셋 (부모 클래스 확장 시 통합 고려) */
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Skill|Values")
	float HitBoxElevationOffset = 20.0f;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Skill|Combo")
	bool bUseRandomCombo = false;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Skill|Primary Targeting", meta = (ClampMin = "0.0"))
	float AutoTargetSearchRadius = 450.0f;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Skill|Primary Targeting", meta = (ClampMin = "0.0"))
	float AutoTargetFacingDuration = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Skill|Movement")
	TSet<int32> ActionMotionComboIndices = { 2 };

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Skill|Movement", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MobileAttackMovementAssistSpeedRatio = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Skill|Movement", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MovingAttackLowerBodyMotionMatchBlendAlpha = 0.65f;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Skill|Movement", meta = (ClampMin = "0.0"))
	float IdlePrimaryComboSpeedThreshold = 15.0f;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Skill|Visuals", meta = (Categories = "GameplayCue", ToolTip = "Cue key used to resolve the active sword trail Niagara from SkillData.VisualCues."))
	FGameplayTag PrimaryTrailCueTag;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Skill|Visuals", meta = (Categories = "GameplayCue", ToolTip = "Cue key used to resolve the hit-frame burst Niagara from SkillData.VisualCues."))
	FGameplayTag PrimaryBurstCueTag;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Skill|Visuals")
	FName PrimaryVFXSocketName = TEXT("hand_r");

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Skill|Visuals")
	FVector PrimaryVFXLocationOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Skill|Visuals")
	FRotator PrimaryVFXRotationOffset = FRotator::ZeroRotator;

private:
	// =========================================================================
	// 3. 콤보 시퀀스 및 상태 제어 함수군 (Combo Sequence Helpers)
	// =========================================================================
	void StartComboSequence();
	int32 GetNextComboIndex(int32 MaxComboCount);
	bool ShouldUseActionMotionTrackingForComboIndex(int32 ComboIndex) const;
	void ClearExistingTasks();
	void ClearEventTasks();
	void ScheduleSourceFallbackMontageCompletion(float Duration);
	void OnSourceFallbackMontageCompleted();
	bool TryStartQueuedAttackFromActionEnd();
	bool IsPrimaryComboStartingFromIdle(const class AGP_PlayerCharacter* PlayerCharacter) const;
	void ApplyForcedPrimaryCrouch(class AGP_PlayerCharacter* PlayerCharacter);
	void RestoreForcedPrimaryCrouch(class AGP_PlayerCharacter* PlayerCharacter);
	const UGP_SkillData* ResolvePrimarySkillData() const;
	UNiagaraSystem* ResolvePrimaryNiagaraSystem(FGameplayTag CueTag, int32 FallbackVisualCueIndex) const;
	void StartPrimaryTrailVFX();
	void StopPrimaryTrailVFX();
	void SpawnPrimaryBurstVFX();

	// =========================================================================
	// 4. 애니메이션 몽타주 및 게임플레이 이벤트 콜백 (Gameplay Event Callbacks)
	// =========================================================================
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

	// =========================================================================
	// 5. 내부 상태 변수군 (Internal Locomotion & Combo States)
	// =========================================================================
	int32 CurrentComboIndex = 0;
	bool bHasQueuedNextAttack = false;
	bool bIsComboWindowOpen = false;
	bool bCurrentSwingUsesActionMotionTracking = false;
	bool bActionEndWindowOpen = false;
	bool bWasCrouchedBeforePrimary = false;
	bool bForcedPrimaryCrouchApplied = false;
	bool bComboStartedFromIdle = false;

	// Some migrated primary montages still send the ability tag as their hit notify, so each swing guards against duplicate hit events.
	bool bHasAppliedCurrentAttackHit = false;

	UPROPERTY()
	TObjectPtr<UAnimMontage> CurrentTargetAttackMontage;

	UPROPERTY()
	TObjectPtr<UAnimMontage> CurrentSourceAttackMontage;

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> ActivePrimaryTrailVFX;

	// =========================================================================
	// 6. GAS 어빌리티 태스크 변수군 (Gameplay Ability Tasks)
	// =========================================================================
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

	FTimerHandle SourceFallbackCompletionTimerHandle;
};
