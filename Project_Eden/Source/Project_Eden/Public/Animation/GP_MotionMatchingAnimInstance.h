#pragma once

#include "CoreMinimal.h"
#include "Animation/GP_CharacterAnimInstance.h"
#include "GP_AnimationTypes.h"
#include "GP_MotionMatchingAnimInstance.generated.h"

class UPoseSearchDatabase;

/**
 * [마이그레이션 단계 4] Motion Matching 전용 애니메이션 인스턴스.
 * 공통 물리 데이터를 기반으로 MM/Chooser에 필요한 전략적 상태를 계산합니다.
 */
UCLASS()
class PROJECT_EDEN_API UGP_MotionMatchingAnimInstance : public UGP_CharacterAnimInstance
{
	GENERATED_BODY()

public:
	UGP_MotionMatchingAnimInstance();

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	/** Chooser 또는 로직에 의해 선택된 현재 포즈 검색 데이터베이스 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "MotionMatching")
	TObjectPtr<UPoseSearchDatabase> SelectedPoseSearchDatabase;

	/** 블루프린트(Chooser 결과 등)에서 DB를 공식적으로 세팅하는 경로 */
	UFUNCTION(BlueprintCallable, Category = "MotionMatching")
	void SetSelectedPoseSearchDatabase(UPoseSearchDatabase* NewDatabase);

	/** 
	 * [단일 진실원] C++ 열거형 기반 상태 변수.
	 * 블루프린트 및 Chooser와의 타입 정렬을 보장합니다.
	 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "MotionMatching|State")
	E_Gait Gait;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "MotionMatching|State")
	E_Stance Stance;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "MotionMatching|State")
	E_MovementMode MovementMode;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "MotionMatching|State")
	E_MovementState MovementState;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "MotionMatching|State")
	bool IsStarting = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "MotionMatching|State")
	bool IsPivoting = false;

public:
	// UI 및 외부 접근을 위한 Getter
	UFUNCTION(BlueprintPure, Category = "MotionMatching|State")
	E_Gait GetGait() const { return Gait; }

	UFUNCTION(BlueprintPure, Category = "MotionMatching|State")
	bool GetIsStarting() const { return IsStarting; }

	UFUNCTION(BlueprintPure, Category = "MotionMatching|State")
	bool GetIsPivoting() const { return IsPivoting; }

	UFUNCTION(BlueprintPure, Category = "MotionMatching|State")
	float GetGroundSpeed() const { return GroundSpeed; }

protected:
	/** Chooser/AnimBP에서 절댓값 처리 없이 바로 쓰도록 제공 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "MotionMatching|State")
	float AbsControlYawDelta = 0.0f;

	/** 로코모션 상태(Gait, Stance 등)를 일괄 갱신하는 함수 */
	UFUNCTION(BlueprintCallable, Category = "MotionMatching|Update")
	virtual void UpdateLocomotionStates();

	/** Chooser를 통해 적절한 PoseSearchDatabase를 선택하는 지점 */
	/**
	 * Chooser를 통해 적절한 PoseSearchDatabase를 선택하는 지점
	 * - 기본 구현은 아무 것도 하지 않습니다.
	 * - AnimBP에서 Override하여 Chooser Evaluate 후 SetSelectedPoseSearchDatabase()를 호출하는 패턴을 의도합니다.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "MotionMatching|Update")
	void UpdateMotionMatchingState();
	virtual void UpdateMotionMatchingState_Implementation();

	// === 디버그 ===
	/** 상태 변화를 로그로 출력할지 여부 (AnimInstance에서만 사용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionMatching|Debug")
	bool bDebugMotionMatchingState = false;

protected:
	/** Turn In Place 튜닝(선택): 정지로 간주할 속도(이하) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MotionMatching|TurnInPlace", meta = (ClampMin = "0.0"))
	float TurnInPlaceSpeedThreshold = 10.0f;

	/** Turn In Place 튜닝(선택): 회전으로 간주할 Yaw Delta(이상, degrees) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MotionMatching|TurnInPlace", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float TurnInPlaceYawThreshold = 45.0f;

	/** Turn In Place 튜닝(선택): 회전으로 간주할 TurnRate(이상, deg/s) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MotionMatching|TurnInPlace", meta = (ClampMin = "0.0"))
	float TurnInPlaceTurnRateThreshold = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionMatching|Thresholds", meta = (ClampMin = "0.0"))
	float IdleSpeedThreshold = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionMatching|Thresholds", meta = (ClampMin = "0.0"))
	float WalkSpeedThreshold = 210.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionMatching|Thresholds", meta = (ClampMin = "0.0"))
	float RunSpeedThreshold = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionMatching|Thresholds", meta = (ClampMin = "0.0"))
	float SprintSpeedThreshold = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionMatching|Thresholds", meta = (ClampMin = "0.0"))
	float StartMinSpeedThreshold = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionMatching|Thresholds", meta = (ClampMin = "0.0"))
	float StartMaxSpeedThreshold = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionMatching|Thresholds", meta = (ClampMin = "0.0"))
	float StartMinAccelerationThreshold = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionMatching|Thresholds", meta = (ClampMin = "0.0"))
	float StartWindowSeconds = 0.22f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionMatching|Thresholds", meta = (ClampMin = "0.0"))
	float PivotMinSpeedThreshold = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionMatching|Thresholds", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float PivotMinAngleDegrees = 70.0f;

private:
	float StartElapsedTime = 0.0f;
	bool bWasMovingLastFrame = false;
};
