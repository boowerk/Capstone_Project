#include "Animation/GP_MotionMatchingAnimInstance.h"
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
	const float MaxSpeed = FMath::Max(MovementComponent->GetMaxSpeed(), 1.0f);
	const float DynamicWalkToRunThreshold = FMath::Clamp(MaxSpeed * WalkToRunSpeedRatio, IdleSpeedThreshold + 1.0f, MaxSpeed);
	const bool bSprintSpeedAvailable = MaxSpeed >= MinimumSprintSpeed;
	const float DynamicRunToSprintThreshold = bSprintSpeedAvailable
		? FMath::Clamp(MaxSpeed * RunToSprintSpeedRatio, DynamicWalkToRunThreshold + 1.0f, MaxSpeed)
		: TNumericLimits<float>::Max();
	const float StartSpeedCeiling = FMath::Min(StartMaxSpeedThreshold, DynamicWalkToRunThreshold);
	const float AccelerationMagnitude = Acceleration.Size2D();
	const bool bHasMeaningfulAcceleration = AccelerationMagnitude >= StartMinAccelerationThreshold;
	const bool bHasDedicatedRunBand = MaxSpeed > MinimumSprintSpeed;
	const bool bIsMovingNow = GroundSpeed >= StartMinSpeedThreshold;
	const bool bJustStartedMoving = !bWasMovingLastFrame && bIsMovingNow && bHasMeaningfulAcceleration;
	const bool bWithinStartWindow = StartElapsedTime <= StartWindowSeconds;

	// 3-1. Resolve stance.
	Stance = bIsCrouching ? E_Stance::Crouch : E_Stance::Stand;

	// 3-2. Resolve movement mode.
	MovementMode = bShouldBeInAir ? E_MovementMode::InAir : E_MovementMode::OnGround;

	// 3-3. Resolve gait from the same dynamic thresholds the sample uses.
	if (Stance == E_Stance::Crouch)
	{
		Gait = E_Gait::Walk;
	}
	else if (bSprintTagActive || (bSprintSpeedAvailable && GroundSpeed >= DynamicRunToSprintThreshold))
	{
		Gait = E_Gait::Sprint;
	}
	else if (bHasDedicatedRunBand && GroundSpeed >= DynamicWalkToRunThreshold)
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

	IsStarting = MovementMode == E_MovementMode::OnGround
		&& MovementState == E_MovementState::Moving
		&& bHasMeaningfulAcceleration
		&& bWithinStartWindow
		&& !bShouldBeInAir
		&& !bWasMovingLastFrame
		&& GroundSpeed >= StartMinSpeedThreshold
		&& GroundSpeed <= StartSpeedCeiling
		&& Gait != E_Gait::Sprint;

	IsPivoting = MovementMode == E_MovementMode::OnGround
		&& MovementState == E_MovementState::Moving
		&& GroundSpeed >= PivotMinSpeedThreshold
		&& bHasMeaningfulAcceleration
		&& FMath::Abs(LocalVelocityAngleDegrees) >= PivotMinAngleDegrees;

	if (MovementState == E_MovementState::Idle)
	{
		IsStarting = false;
		IsPivoting = false;
	}

	bWasMovingLastFrame = bIsMovingNow;

	if (bDebugMotionMatchingState)
	{
		if (PrevGait != Gait || PrevStance != Stance || PrevMovementMode != MovementMode || PrevMovementState != MovementState)
		{
			UE_LOG(LogTemp, Log, TEXT("[MMState] Gait:%d->%d Stance:%d->%d Mode:%d->%d State:%d->%d Speed:%.1f WalkToRun:%.1f RunToSprint:%.1f SprintTag:%d Falling:%d Crouch:%d"),
				(int32)PrevGait, (int32)Gait,
				(int32)PrevStance, (int32)Stance,
				(int32)PrevMovementMode, (int32)MovementMode,
				(int32)PrevMovementState, (int32)MovementState,
				GroundSpeed,
				DynamicWalkToRunThreshold,
				bSprintSpeedAvailable ? DynamicRunToSprintThreshold : -1.0f,
				bSprintTagActive ? 1 : 0,
				bShouldBeInAir ? 1 : 0,
				bIsCrouching ? 1 : 0);
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

