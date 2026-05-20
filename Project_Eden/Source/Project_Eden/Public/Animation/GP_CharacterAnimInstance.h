#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/TrajectoryTypes.h"
#include "PDA_CharacterAnimationSet.h"
#include "PoseSearch/PoseSearchTrajectoryLibrary.h"
#include "GP_CharacterAnimInstance.generated.h"

class ACharacter;
class AGP_PlayerCharacter;
class UCharacterMovementComponent;
class UBlendSpace;
class UAnimSequenceBase;
class UPoseSearchDatabase;

UENUM(BlueprintType)
enum class ESourceMotionMatchState : uint8
{
	Idle,
	Walk,
	Run,
	Sprint,
	Jump
};

/**
 * 프로젝트의 모든 캐릭터 애니메이션 블루프린트를 위한 공통 베이스 클래스.
 */
UCLASS()
class PROJECT_EDEN_API UGP_CharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UGP_CharacterAnimInstance();
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	/** 외부(캐릭터 등)에서 애니메이션 세트를 동적으로 주입하기 위한 함수 */
	UFUNCTION(BlueprintCallable, Category = "AnimationData")
	void SetAnimationSet(UPDA_CharacterAnimationSet* NewSet);

protected:
	/** 캐릭터별 애니메이션 세트 에셋 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnimationData")
	TObjectPtr<UPDA_CharacterAnimationSet> AnimationSet;

	// === Runtime Cached Assets (AnimGraph에서 직접 사용) ===
	
	UPROPERTY(BlueprintReadOnly, Category = "AnimationData|Cached")
	TObjectPtr<UBlendSpace> LocomotionBlendSpace;

	UPROPERTY(BlueprintReadOnly, Category = "AnimationData|Cached")
	TObjectPtr<UAnimSequenceBase> JumpLoopAnimation;

	UPROPERTY(BlueprintReadOnly, Category = "AnimationData|Cached")
	TObjectPtr<UAnimSequenceBase> SprintStopAnimation;

	/** 현재 소유 중인 캐릭터 캐시 */
	UPROPERTY(BlueprintReadOnly, Category = "Character")
	TObjectPtr<ACharacter> Character;

	/** 캐릭터의 무브먼트 컴포넌트 캐시 */
	UPROPERTY(BlueprintReadOnly, Category = "Character")
	TObjectPtr<UCharacterMovementComponent> MovementComponent;

	// === Locomotion Data (캐릭터 공통) ===
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float GroundSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	FVector LocalVelocityDirection;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bIsFalling;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bHasAcceleration;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bIsAnyMontagePlaying;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bIsSprinting;

	UPROPERTY(BlueprintReadOnly, Category = "MotionMatching")
	FTransformTrajectory GeneratedTrajectory;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching")
	FPoseSearchTrajectoryData TrajectoryData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching", meta = (ClampMin = "0.0"))
	float TrajectoryHistorySamplingInterval = 0.04f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching", meta = (ClampMin = "0"))
	int32 TrajectoryHistoryCount = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching", meta = (ClampMin = "0.0"))
	float TrajectoryPredictionSamplingInterval = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching", meta = (ClampMin = "0"))
	int32 TrajectoryPredictionCount = 8;

	UPROPERTY(BlueprintReadOnly, Category = "MotionMatching")
	float DesiredControllerYawLastUpdate = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching|Databases")
	TObjectPtr<UPoseSearchDatabase> IdlePoseSearchDatabase;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching|Databases")
	TObjectPtr<UPoseSearchDatabase> WalkPoseSearchDatabase;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching|Databases")
	TObjectPtr<UPoseSearchDatabase> RunPoseSearchDatabase;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching|Databases")
	TObjectPtr<UPoseSearchDatabase> SprintPoseSearchDatabase;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching|Databases")
	TObjectPtr<UPoseSearchDatabase> JumpPoseSearchDatabase;

	UPROPERTY(BlueprintReadOnly, Category = "MotionMatching|Databases")
	TObjectPtr<UPoseSearchDatabase> RuntimePoseSearchDatabase;

	UPROPERTY(BlueprintReadOnly, Category = "MotionMatching|State")
	ESourceMotionMatchState CurrentMotionMatchState = ESourceMotionMatchState::Idle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching|Databases", meta = (ClampMin = "0.0"))
	float IdleSpeedThreshold = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching|Databases", meta = (ClampMin = "0.0"))
	float WalkSpeedThreshold = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching|Databases", meta = (ClampMin = "0.0"))
	float RunSpeedThreshold = 360.f;
};
