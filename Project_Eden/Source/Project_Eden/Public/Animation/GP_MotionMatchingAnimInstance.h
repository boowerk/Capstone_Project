#pragma once

#include "CoreMinimal.h"
#include "Animation/GP_CharacterAnimInstance.h"
#include "GP_AnimationTypes.h"
#include "GP_MotionMatchingAnimInstance.generated.h"

class UPoseSearchDatabase;

/**
 * [마이그레이션 단계 4] Motion Matching 전용 애님 인스턴스.
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
	 * [단일 진실원] C++ 에넘 기반 상태 변수.
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
	// === 상태 계산용 임계값 (상수) ===
	
	/** 정지 상태로 간주할 최대 속도 */
	static constexpr float IdleSpeedThreshold = 5.0f;

	/** 걷기에서 달리기로 전환되는 속도 임계값 */
	static constexpr float WalkToRunThreshold = 250.0f;

	/** 달리기에서 전력 질주로 전환되는 속도 임계값 */
	static constexpr float RunToSprintThreshold = 550.0f;
};
