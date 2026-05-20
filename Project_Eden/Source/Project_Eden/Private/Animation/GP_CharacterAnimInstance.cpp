#include "Animation/GP_CharacterAnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PoseSearch/PoseSearchDatabase.h"

UGP_CharacterAnimInstance::UGP_CharacterAnimInstance()
{
	// 스테이트 머신의 시퀀스에서도 루트 모션이 작동하도록 설정
	RootMotionMode = ERootMotionMode::RootMotionFromEverything;
}

void UGP_CharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Character = Cast<ACharacter>(TryGetPawnOwner());
	if (Character)
	{
		MovementComponent = Character->GetCharacterMovement();
	}
}

void UGP_CharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!Character || !MovementComponent)
	{
		// 유효하지 않을 경우 재시도
		Character = Cast<ACharacter>(TryGetPawnOwner());
		if (Character)
		{
			MovementComponent = Character->GetCharacterMovement();
		}
	}

	// 데이터 에셋으로부터 애니메이션 캐싱 (동적 교체 지원)
	if (AnimationSet)
	{
		LocomotionBlendSpace = AnimationSet->LocomotionBlendSpace;
		JumpLoopAnimation = AnimationSet->JumpLoopAnimation;
		SprintStopAnimation = AnimationSet->SprintStopAnimation;
	}

	if (!Character || !MovementComponent) return;

	// 속도 및 이동 데이터 추출
	FVector WorldVelocity = Character->GetVelocity();
	GroundSpeed = WorldVelocity.Size2D();
	
	// 캐릭터 로컬 방향으로 변환
	LocalVelocityDirection = Character->GetActorTransform().InverseTransformVectorNoScale(WorldVelocity).GetSafeNormal2D();

	// 상태 데이터 업데이트
	bHasAcceleration = MovementComponent->GetCurrentAcceleration().SizeSquared2D() > KINDA_SMALL_NUMBER;
	bIsFalling = MovementComponent->IsFalling();
	bIsAnyMontagePlaying = Montage_IsPlaying(nullptr);
	const float CurrentMaxSpeed = MovementComponent->GetMaxSpeed();
	bIsSprinting = CurrentMaxSpeed > RunSpeedThreshold;

	FTransformTrajectory UpdatedTrajectory;
	UPoseSearchTrajectoryLibrary::PoseSearchGenerateTransformTrajectory(
		this,
		TrajectoryData,
		DeltaSeconds,
		GeneratedTrajectory,
		DesiredControllerYawLastUpdate,
		UpdatedTrajectory,
		TrajectoryHistorySamplingInterval,
		TrajectoryHistoryCount,
		TrajectoryPredictionSamplingInterval,
		TrajectoryPredictionCount);
	GeneratedTrajectory = MoveTemp(UpdatedTrajectory);

	if (bIsFalling && JumpPoseSearchDatabase)
	{
		CurrentMotionMatchState = ESourceMotionMatchState::Jump;
		RuntimePoseSearchDatabase = JumpPoseSearchDatabase;
	}
	else if (bIsSprinting && (GroundSpeed > IdleSpeedThreshold || bHasAcceleration) && SprintPoseSearchDatabase)
	{
		CurrentMotionMatchState = ESourceMotionMatchState::Sprint;
		RuntimePoseSearchDatabase = SprintPoseSearchDatabase;
	}
	else if (GroundSpeed <= IdleSpeedThreshold && !bHasAcceleration && IdlePoseSearchDatabase)
	{
		CurrentMotionMatchState = ESourceMotionMatchState::Idle;
		RuntimePoseSearchDatabase = IdlePoseSearchDatabase;
	}
	else if (GroundSpeed <= WalkSpeedThreshold && WalkPoseSearchDatabase)
	{
		CurrentMotionMatchState = ESourceMotionMatchState::Walk;
		RuntimePoseSearchDatabase = WalkPoseSearchDatabase;
	}
	else if (GroundSpeed <= RunSpeedThreshold && RunPoseSearchDatabase)
	{
		CurrentMotionMatchState = ESourceMotionMatchState::Run;
		RuntimePoseSearchDatabase = RunPoseSearchDatabase;
	}
	else if (SprintPoseSearchDatabase)
	{
		CurrentMotionMatchState = ESourceMotionMatchState::Sprint;
		RuntimePoseSearchDatabase = SprintPoseSearchDatabase;
	}
	else if (RunPoseSearchDatabase)
	{
		CurrentMotionMatchState = ESourceMotionMatchState::Run;
		RuntimePoseSearchDatabase = RunPoseSearchDatabase;
	}
	else if (WalkPoseSearchDatabase)
	{
		CurrentMotionMatchState = ESourceMotionMatchState::Walk;
		RuntimePoseSearchDatabase = WalkPoseSearchDatabase;
	}
	else
	{
		CurrentMotionMatchState = ESourceMotionMatchState::Idle;
		RuntimePoseSearchDatabase = IdlePoseSearchDatabase;
	}
}

void UGP_CharacterAnimInstance::SetAnimationSet(UPDA_CharacterAnimationSet* NewSet)
{
	if (NewSet)
	{
		AnimationSet = NewSet;
		
		// 캐시 즉시 갱신
		LocomotionBlendSpace = AnimationSet->LocomotionBlendSpace;
		JumpLoopAnimation = AnimationSet->JumpLoopAnimation;
		SprintStopAnimation = AnimationSet->SprintStopAnimation;
	}
}
