#include "Animation/GP_CharacterAnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/GP_PlayerController.h"

UGP_CharacterAnimInstance::UGP_CharacterAnimInstance()
{
	// 스테이트 머신의 시퀀스에서도 루트 모션이 작동하도록 설정
	RootMotionMode = ERootMotionMode::RootMotionFromEverything;
	LocalVelocityAngleDegrees = 0.0f;
	LocalAccelerationAngleDegrees = 0.0f;
	MoveInput = FVector2D::ZeroVector;
	bHasMoveInput = false;
	ControlYawDelta = 0.0f;
	ControlYawDeltaRate = 0.0f;
	TurnRate = 0.0f;
}

void UGP_CharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Character = Cast<ACharacter>(TryGetPawnOwner());
	if (Character)
	{
		MovementComponent = Character->GetCharacterMovement();
		PreviousActorYaw = Character->GetActorRotation().Yaw;
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
			PreviousActorYaw = Character->GetActorRotation().Yaw;
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
	Velocity = Character->GetVelocity();
	Acceleration = MovementComponent->GetCurrentAcceleration();
	GroundSpeed = Velocity.Size2D();
	
	// 캐릭터 로컬 방향으로 변환
	const FVector LocalVelocity = Character->GetActorTransform().InverseTransformVectorNoScale(Velocity);
	LocalVelocityDirection = LocalVelocity.GetSafeNormal2D();
	LocalVelocityAngleDegrees = FMath::RadiansToDegrees(FMath::Atan2(LocalVelocity.Y, LocalVelocity.X));

	const FVector LocalAcceleration = Character->GetActorTransform().InverseTransformVectorNoScale(Acceleration);
	LocalAccelerationAngleDegrees = FMath::RadiansToDegrees(FMath::Atan2(LocalAcceleration.Y, LocalAcceleration.X));

	const float ActorYaw = Character->GetActorRotation().Yaw;
	TurnRate = DeltaSeconds > KINDA_SMALL_NUMBER
		? FMath::FindDeltaAngleDegrees(PreviousActorYaw, ActorYaw) / DeltaSeconds
		: 0.0f;
	PreviousActorYaw = ActorYaw;

	if (const AGP_PlayerController* PlayerController = Cast<AGP_PlayerController>(Character->GetController()))
	{
		MoveInput = PlayerController->GetCurrentMoveInput();
		bHasMoveInput = !MoveInput.IsNearlyZero();

		const float CurrentControlYawDelta = FMath::FindDeltaAngleDegrees(ActorYaw, PlayerController->GetControlRotation().Yaw);
		ControlYawDeltaRate = DeltaSeconds > KINDA_SMALL_NUMBER
			? (CurrentControlYawDelta - PreviousControlYawDelta) / DeltaSeconds
			: 0.0f;
		ControlYawDelta = CurrentControlYawDelta;
		PreviousControlYawDelta = CurrentControlYawDelta;
	}
	else
	{
		MoveInput = FVector2D::ZeroVector;
		bHasMoveInput = false;
		ControlYawDelta = 0.0f;
		ControlYawDeltaRate = 0.0f;
		PreviousControlYawDelta = 0.0f;
	}

	// 상태 데이터 업데이트
	bHasAcceleration = Acceleration.SizeSquared2D() > KINDA_SMALL_NUMBER;
	bIsFalling = MovementComponent->IsFalling();
	bIsCrouching = MovementComponent->IsCrouching();
	bIsAnyMontagePlaying = Montage_IsPlaying(nullptr);
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
