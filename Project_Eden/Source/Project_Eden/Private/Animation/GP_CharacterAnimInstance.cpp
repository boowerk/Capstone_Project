#include "Animation/GP_CharacterAnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/GP_PlayerController.h"

UGP_CharacterAnimInstance::UGP_CharacterAnimInstance()
{
	// Let character movement drive locomotion. Reserve root motion for montages.
	RootMotionMode = ERootMotionMode::RootMotionFromMontagesOnly;
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
		// Retry if the cached owner is not valid yet.
		Character = Cast<ACharacter>(TryGetPawnOwner());
		if (Character)
		{
			MovementComponent = Character->GetCharacterMovement();
			PreviousActorYaw = Character->GetActorRotation().Yaw;
		}
	}

	// Refresh animation asset references from the data asset.
	if (AnimationSet && bForceApplyAnimationSet)
	{
		LocomotionBlendSpace = AnimationSet->LocomotionBlendSpace;
		JumpLoopAnimation = AnimationSet->JumpLoopAnimation;
		SprintStopAnimation = AnimationSet->SprintStopAnimation;
	}

	if (!Character || !MovementComponent) return;

	// Gather locomotion data from the owning character.
	Velocity = Character->GetVelocity();
	Acceleration = MovementComponent->GetCurrentAcceleration();
	GroundSpeed = Velocity.Size2D();
	
	const float ActorYaw = Character->GetActorRotation().Yaw;
	TurnRate = DeltaSeconds > KINDA_SMALL_NUMBER
		? FMath::FindDeltaAngleDegrees(PreviousActorYaw, ActorYaw) / DeltaSeconds
		: 0.0f;
	PreviousActorYaw = ActorYaw;

	if (const AGP_PlayerController* PlayerController = Cast<AGP_PlayerController>(Character->GetController()))
	{
		const FRotator ReferenceRotation(0.0f, PlayerController->GetControlRotation().Yaw, 0.0f);
		const FVector LocalVelocity = ReferenceRotation.UnrotateVector(Velocity);
		LocalVelocityDirection = LocalVelocity.GetSafeNormal2D();
		LocalVelocityAngleDegrees = FMath::RadiansToDegrees(FMath::Atan2(LocalVelocity.Y, LocalVelocity.X));

		const FVector LocalAcceleration = ReferenceRotation.UnrotateVector(Acceleration);
		LocalAccelerationAngleDegrees = FMath::RadiansToDegrees(FMath::Atan2(LocalAcceleration.Y, LocalAcceleration.X));

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
		const FVector LocalVelocity = Character->GetActorTransform().InverseTransformVectorNoScale(Velocity);
		LocalVelocityDirection = LocalVelocity.GetSafeNormal2D();
		LocalVelocityAngleDegrees = FMath::RadiansToDegrees(FMath::Atan2(LocalVelocity.Y, LocalVelocity.X));

		const FVector LocalAcceleration = Character->GetActorTransform().InverseTransformVectorNoScale(Acceleration);
		LocalAccelerationAngleDegrees = FMath::RadiansToDegrees(FMath::Atan2(LocalAcceleration.Y, LocalAcceleration.X));

		MoveInput = FVector2D::ZeroVector;
		bHasMoveInput = false;
		ControlYawDelta = 0.0f;
		ControlYawDeltaRate = 0.0f;
		PreviousControlYawDelta = 0.0f;
	}

	// Update high-level locomotion flags.
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
		
		// Refresh cached references immediately if force apply is enabled.
		if (bForceApplyAnimationSet)
		{
			LocomotionBlendSpace = AnimationSet->LocomotionBlendSpace;
			JumpLoopAnimation = AnimationSet->JumpLoopAnimation;
			SprintStopAnimation = AnimationSet->SprintStopAnimation;
		}
	}
}

