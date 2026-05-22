#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimExecutionContext.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimNodeReference.h"
#include "Animation/TrajectoryTypes.h"
#include "PDA_CharacterAnimationSet.h"
#include "PoseSearch/PoseSearchLibrary.h"
#include "PoseSearch/MotionMatchingAnimNodeLibrary.h"
#include "PoseSearch/PoseSearchTrajectoryLibrary.h"
#include "GP_CharacterAnimInstance.generated.h"

class ACharacter;
class AGP_PlayerCharacter;
class UCharacterMovementComponent;
class UBlendSpace;
class UAnimSequenceBase;
class UChooserTable;
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

UENUM(BlueprintType)
enum class EMMDatabaseLOD : uint8
{
	Dense = 0,
	Sparse = 1,
	ExtremeSparse = 2
};

UENUM(BlueprintType)
enum class E_MovementMode : uint8
{
	Grounded = 0,
	Slide = 1,
	InAir = 2
};

UENUM(BlueprintType)
enum class E_Stance : uint8
{
	Standing = 0,
	Crouching = 1
};

UENUM(BlueprintType)
enum class E_MovementState : uint8
{
	Idle = 0,
	Moving = 1
};

UENUM(BlueprintType)
enum class E_Gait : uint8
{
	Walk = 0,
	Run = 1,
	Sprint = 2
};

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class E_MovementDirection : uint8
{
	Forward = 0,
	Backward = 1,
	Left = 2,
	Right = 4
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

	UFUNCTION(BlueprintCallable, Category = "MotionMatching", meta = (BlueprintThreadSafe))
	void ApplyRuntimeDatabaseToMotionMatchingNode(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);

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

	UPROPERTY(BlueprintReadOnly, Category = "Chooser")
	float Speed2D = 0.f;

	// Root chooser compatibility with the original sample's LOD branch.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chooser")
	float MMDatabaseLOD = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chooser")
	EMMDatabaseLOD MMDatabaseLODEnum = EMMDatabaseLOD::ExtremeSparse;

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

	UPROPERTY(BlueprintReadOnly, Category = "Chooser")
	E_MovementMode MovementMode = E_MovementMode::Grounded;

	UPROPERTY(BlueprintReadOnly, Category = "Chooser")
	E_MovementMode MovementMode_LastFrame = E_MovementMode::Grounded;

	UPROPERTY(BlueprintReadOnly, Category = "Chooser")
	E_Stance Stance = E_Stance::Standing;

	UPROPERTY(BlueprintReadOnly, Category = "Chooser")
	E_MovementState MovementState = E_MovementState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Chooser")
	E_Gait Gait = E_Gait::Walk;

	UPROPERTY(BlueprintReadOnly, Category = "Chooser")
	E_Gait Gait_LastFrame = E_Gait::Walk;

	UPROPERTY(BlueprintReadOnly, Category = "Chooser")
	E_MovementDirection MovementDirection = E_MovementDirection::Forward;

	UPROPERTY(BlueprintReadOnly, Category = "Chooser")
	uint8 MovementDirection_Recent = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Chooser")
	bool IsStarting = false;

	UPROPERTY(BlueprintReadOnly, Category = "Chooser")
	bool IsStopping = false;

	UPROPERTY(BlueprintReadOnly, Category = "Chooser")
	bool IsPivoting = false;

	UPROPERTY(BlueprintReadOnly, Category = "Chooser")
	bool ShouldSpinTransition = false;

	UPROPERTY(BlueprintReadOnly, Category = "Chooser")
	bool JustTraversed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Chooser")
	bool JustLanded_Light = false;

	UPROPERTY(BlueprintReadOnly, Category = "Chooser")
	bool JustLanded_Heavy = false;

	UPROPERTY(BlueprintReadOnly, Category = "Chooser")
	bool ShouldTurnInPlace = false;

	// Original chooser uses TimeToLand in the InAir branch.
	UPROPERTY(BlueprintReadOnly, Category = "Chooser")
	float TimeToLand = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "MotionMatching")
	FTransformTrajectory GeneratedTrajectory;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching")
	FPoseSearchTrajectoryData TrajectoryData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching", meta = (ClampMin = "0.0"))
	float TrajectoryHistorySamplingInterval = 0.04f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching", meta = (ClampMin = "0"))
	int32 TrajectoryHistoryCount = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching", meta = (ClampMin = "0.0"))
	float TrajectoryPredictionSamplingInterval = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching", meta = (ClampMin = "0"))
	int32 TrajectoryPredictionCount = 12;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching|Chooser")
	TObjectPtr<UChooserTable> PoseSearchChooser;

	UPROPERTY(BlueprintReadOnly, Category = "MotionMatching|Databases")
	TObjectPtr<UPoseSearchDatabase> RuntimePoseSearchDatabase;

	UPROPERTY(BlueprintReadOnly, Category = "MotionMatching|State")
	ESourceMotionMatchState CurrentMotionMatchState = ESourceMotionMatchState::Idle;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "MotionMatching|State")
	TObjectPtr<UPoseSearchDatabase> LastAppliedRuntimePoseSearchDatabase;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "MotionMatching|Debug")
	bool bMotionMatchingResultValid = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "MotionMatching|Debug")
	FName MotionMatchingSelectedAnimName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching|Databases", meta = (ClampMin = "0.0"))
	float IdleSpeedThreshold = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching|Databases", meta = (ClampMin = "0.0"))
	float WalkSpeedThreshold = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching|Databases", meta = (ClampMin = "0.0"))
	float RunSpeedThreshold = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching|Databases", meta = (ClampMin = "0.0"))
	float SprintSpeedThreshold = 650.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching|State", meta = (ClampMin = "0.0"))
	float StopSpeedThreshold = 140.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching|State", meta = (ClampMin = "0.0"))
	float TurnInPlaceMaxSpeed = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching|State", meta = (ClampMin = "0.0"))
	float TurnInPlaceYawThreshold = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching|State", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float PivotDirectionDotThreshold = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching|State", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float PivotExitDirectionDotThreshold = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching|State", meta = (ClampMin = "0.0"))
	float MovingTurnYawRateThreshold = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching|State", meta = (ClampMin = "0.0"))
	float TurnInPlaceMinIdleTime = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching|State", meta = (ClampMin = "0.0"))
	float LandedSignalDuration = 0.15f;

	UPROPERTY(BlueprintReadOnly, Category = "MotionMatching|State")
	bool bWasMovingLastFrame = false;

	UPROPERTY(BlueprintReadOnly, Category = "MotionMatching|State", meta = (ClampMin = "0.0"))
	float TimeSinceMovementStarted = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "MotionMatching|State", meta = (ClampMin = "0.0"))
	float TimeSinceMovementStopped = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "MotionMatching|State", meta = (ClampMin = "0.0"))
	float TimeSinceStopStarted = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "MotionMatching|State", meta = (ClampMin = "0.0"))
	float TimeSincePivotStarted = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "MotionMatching|State", meta = (ClampMin = "0.0"))
	float TimeSinceLastLanded = 999.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching|State", meta = (ClampMin = "0.0"))
	float MovementStartGraceTime = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching|State", meta = (ClampMin = "0.0"))
	float StopHoldDuration = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching|State", meta = (ClampMin = "0.0"))
	float PivotHoldDuration = 0.2f;

	UPROPERTY(BlueprintReadOnly, Category = "Chooser|State")
	FVector LastLocalVelocityDirection = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Chooser|State")
	float LastVerticalVelocity = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Chooser|State")
	float LastActorYaw = 0.f;

	void ApplyChosenDatabase(UPoseSearchDatabase* SelectedDatabase);
};
