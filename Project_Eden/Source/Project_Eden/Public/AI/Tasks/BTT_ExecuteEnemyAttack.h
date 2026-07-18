#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTagContainer.h"
#include "BTT_ExecuteEnemyAttack.generated.h"

class AActor;
class AGP_EnemyCharacter;
class APawn;
class UAbilitySystemComponent;
class UBehaviorTreeComponent;
class UCharacterMovementComponent;

enum class EEnemyAttackTaskPhase : uint8
{
	Idle,
	FacingTarget,
	AwaitingAbilityEnd,
	FallbackCommit,
	Recovery
};

UCLASS()
class PROJECT_EDEN_API UBTT_ExecuteEnemyAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTT_ExecuteEnemyAttack();

protected:
	// 공유 BT는 이 태그를 가진 적 공격 어빌리티를 실행한다.
	UPROPERTY(EditAnywhere, Category = "AI")
	FGameplayTag AttackAbilityTag;

	// 공격 직전에 이동을 멈춰 과도한 슬라이딩을 줄인다.
	UPROPERTY(EditAnywhere, Category = "AI")
	bool bStopMovementBeforeAttack = true;

	// 공격 직전에 현재 타겟을 바라보도록 회전시킨다.
	UPROPERTY(EditAnywhere, Category = "AI")
	bool bFaceTargetBeforeAttack = true;

	// 보스가 휘둘러치기 가능 상태일 때 기본 공격 대신 휘둘러치기를 선택할 확률입니다.
	UPROPERTY(EditAnywhere, Category = "AI|Boss", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BossSweepAttackChance = 0.5f;

	// 몽타주가 없는 프로토타입 공격도 한 프레임 만에 이동으로 튀지 않도록 짧게 커밋한다.
	UPROPERTY(EditAnywhere, Category = "AI|Transition", meta = (ClampMin = "0.0", Units = "s"))
	float InstantAttackCommitSeconds = 0.2f;

	// ActionEnd 이후 짧은 회복 비트를 두어 공격과 다음 이동이 같은 프레임에 겹치지 않게 한다.
	UPROPERTY(EditAnywhere, Category = "AI|Transition", meta = (ClampMin = "0.0", Units = "s"))
	float AttackRecoverySeconds = 0.18f;

	// 누락된 ActionEnd나 종료 콜백이 BT를 영구 정지시키지 않게 하는 안전 타임아웃이다.
	UPROPERTY(EditAnywhere, Category = "AI|Transition", meta = (ClampMin = "0.1", Units = "s"))
	float AttackTimeoutSeconds = 8.0f;

	// 공격 전 한 프레임 스냅 대신 제한된 각속도로 목표를 바라본다.
	UPROPERTY(EditAnywhere, Category = "AI|Transition|Facing", meta = (ClampMin = "0.0", Units = "deg/s"))
	float AttackFacingTurnRateDegreesPerSecond = 540.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Transition|Facing", meta = (ClampMin = "0.0", ClampMax = "180.0", Units = "deg"))
	float AttackFacingToleranceDegrees = 8.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Transition|Facing", meta = (ClampMin = "0.0", Units = "s"))
	float MaximumAttackFacingSeconds = 0.35f;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnTaskFinished(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory,
		EBTNodeResult::Type TaskResult) override;
	virtual FString GetStaticDescription() const override;

private:
	bool ActivateBasicAttack();
	bool IsTrackedAbilityActive() const;
	bool IsFacingCommittedTarget() const;
	void TickFacingTarget(UBehaviorTreeComponent& OwnerComp, float DeltaSeconds);
	void BeginFacingOwnership(APawn* ControlledPawn);
	void RestoreFacingOwnership();
	void CompleteBasicAttack(UBehaviorTreeComponent& OwnerComp);
	void ScheduleBasicAttackCadence();
	void ResetExecutionState();

	EEnemyAttackTaskPhase ExecutionPhase = EEnemyAttackTaskPhase::Idle;
	TWeakObjectPtr<UAbilitySystemComponent> ActiveAbilitySystemComponent;
	TWeakObjectPtr<AGP_EnemyCharacter> ActiveEnemyCharacter;
	TWeakObjectPtr<APawn> ActiveControlledPawn;
	TWeakObjectPtr<AActor> CommittedTargetActor;
	TWeakObjectPtr<UCharacterMovementComponent> FacingMovementComponent;
	FGameplayTag ActiveAbilityTag;
	float PhaseElapsedSeconds = 0.0f;
	float TotalElapsedSeconds = 0.0f;
	bool bAttackActivationAccepted = false;
	bool bCadenceScheduled = false;
	bool bPreviousOrientRotationToMovement = false;
	bool bPreviousUseControllerDesiredRotation = false;
	bool bOwnsFacingRotation = false;
};
