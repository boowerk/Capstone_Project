#include "Animation/GP_MotionMatchingAnimInstance.h"
#include "UI/GP_MotionMatchingDebugWidget.h"
#include "Blueprint/UserWidget.h"
#include "PoseSearch/PoseSearchDatabase.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayTags/GP_Tags.h"

namespace
{
	bool HasSprintTag(const ACharacter* Character)
	{
		const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(Character);
		const UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemInterface ? AbilitySystemInterface->GetAbilitySystemComponent() : nullptr;
		return AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(GPTags::State::Movement::Sprinting);
	}
}

UGP_MotionMatchingAnimInstance::UGP_MotionMatchingAnimInstance()
{
	// Initialize conservative defaults before the first update.
	Gait = E_Gait::Walk;
	Stance = E_Stance::Stand;
	MovementMode = E_MovementMode::OnGround;
	MovementState = E_MovementState::Idle;
}

void UGP_MotionMatchingAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	// 1. Refresh shared locomotion data first.
	Super::NativeUpdateAnimation(DeltaSeconds);
	StartElapsedTime += DeltaSeconds;

	// 2. Abort if the pawn or movement component is still unavailable.
	if (!Character || !MovementComponent) return;

	// 3. Evaluate locomotion state for motion matching.
	UpdateLocomotionStates();

	// 4. Let the chooser select the active pose database.
	UpdateMotionMatchingState();
}

void UGP_MotionMatchingAnimInstance::UpdateLocomotionStates()
{
	const E_Gait PrevGait = Gait;
	const E_Stance PrevStance = Stance;
	const E_MovementMode PrevMovementMode = MovementMode;
	const E_MovementState PrevMovementState = MovementState;

	AbsControlYawDelta = FMath::Abs(ControlYawDelta);
	const EMovementMode NativeMovementMode = MovementComponent->MovementMode;
	const bool bShouldBeInAir = bIsFalling || NativeMovementMode == MOVE_Falling;
	const bool bSprintTagActive = HasSprintTag(Character);

	const float AccelerationMagnitude = Acceleration.Size2D();
	const bool bHasMeaningfulAcceleration = AccelerationMagnitude >= StartMinAccelerationThreshold;
	const bool bIsMovingNow = GroundSpeed >= StartMinSpeedThreshold;
	const bool bJustStartedMoving = !bWasMovingLastFrame && bIsMovingNow && bHasMeaningfulAcceleration;
	const bool bWithinStartWindow = StartElapsedTime <= StartWindowSeconds;

	// 3-1. Resolve stance.
	Stance = bIsCrouching ? E_Stance::Crouch : E_Stance::Stand;

	// 3-2. Resolve movement mode.
	MovementMode = bShouldBeInAir ? E_MovementMode::InAir : E_MovementMode::OnGround;

	// 3-3. Resolve gait based on absolute thresholds.
	if (Stance == E_Stance::Crouch)
	{
		Gait = E_Gait::Walk;
	}
	else if (bSprintTagActive || GroundSpeed >= RunSpeedThreshold)
	{
		Gait = E_Gait::Sprint;
	}
	else if (GroundSpeed >= WalkSpeedThreshold)
	{
		Gait = E_Gait::Run;
	}
	else
	{
		Gait = E_Gait::Walk;
	}

	// 3-4. Resolve movement state.
	if (MovementMode == E_MovementMode::InAir)
	{
		// Treat any airborne state as moving for chooser purposes.
		MovementState = E_MovementState::Moving;
	}
	else if (GroundSpeed < IdleSpeedThreshold)
	{
		MovementState = E_MovementState::Idle;
	}
	else
	{
		MovementState = E_MovementState::Moving;
	}

	if (bJustStartedMoving)
	{
		StartElapsedTime = 0.0f;
	}

	// 3-5. Resolve Start and Pivot states based on specific criteria.
	const bool bValidStartSpeed = GroundSpeed >= StartMinSpeedThreshold && GroundSpeed <= StartMaxSpeedThreshold;
	
	IsStarting = bWithinStartWindow
		&& MovementMode == E_MovementMode::OnGround
		&& MovementState == E_MovementState::Moving
		&& bValidStartSpeed
		&& Gait != E_Gait::Sprint;

	// Pivot is active when turning sharply at sufficient speed.
	IsPivoting = MovementMode == E_MovementMode::OnGround
		&& MovementState == E_MovementState::Moving
		&& GroundSpeed >= PivotMinSpeedThreshold
		&& bHasMeaningfulAcceleration
		&& FMath::Abs(LocalVelocityAngleDegrees) >= PivotMinAngleDegrees;

	// Force clear states if idle.
	if (MovementState == E_MovementState::Idle)
	{
		IsStarting = false;
		IsPivoting = false;
		StartElapsedTime = StartWindowSeconds + 1.0f; // Reset window
	}

	bWasMovingLastFrame = bIsMovingNow;

	if (bDebugMotionMatchingState)
	{
		if (PrevGait != Gait || PrevStance != Stance || PrevMovementMode != MovementMode || PrevMovementState != MovementState)
		{
			UE_LOG(LogTemp, Log, TEXT("[MMState] Gait:%d->%d Stance:%d->%d Mode:%d->%d State:%d->%d Speed:%.1f Thresholds(W:%.1f, R:%.1f, S:%.1f) SprintTag:%d"),
				(int32)PrevGait, (int32)Gait,
				(int32)PrevStance, (int32)Stance,
				(int32)PrevMovementMode, (int32)MovementMode,
				(int32)PrevMovementState, (int32)MovementState,
				GroundSpeed,
				WalkSpeedThreshold,
				RunSpeedThreshold,
				SprintSpeedThreshold,
				bSprintTagActive ? 1 : 0);
		}
	}
}

void UGP_MotionMatchingAnimInstance::SetSelectedPoseSearchDatabase(UPoseSearchDatabase* NewDatabase)
{
	SelectedPoseSearchDatabase = NewDatabase;
}

void UGP_MotionMatchingAnimInstance::UpdateMotionMatchingState_Implementation()
{
	// Anim blueprints override this hook and assign the active pose database.
}

