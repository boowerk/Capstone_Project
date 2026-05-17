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
	Gait = E_Gait::Walk;
	Stance = E_Stance::Stand;
	MovementMode = E_MovementMode::OnGround;
	MovementState = E_MovementState::Idle;
	ExperimentalState = E_ExperimentalStateMachineState::Idle_Loop;
	IdleUpdateTimer = 0.0f;
	bCanUpdateChooser = true;
	RootMotionMode = ERootMotionMode::IgnoreRootMotion;
}

void UGP_MotionMatchingAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	
	if (!Character || !MovementComponent) return;

	StartElapsedTime += DeltaSeconds;
	
	// 1. Update core locomotion states
	UpdateLocomotionStates();

	// 2. Control Chooser evaluation frequency (Anti-Jitter)
	bool bShouldLockChooser = false;

	// Transition Lock: 전이 중에는 절대로 에셋을 갈아치우지 않습니다. (PSD 기반 전이라도 잠금은 필요)
	if (ExperimentalState == E_ExperimentalStateMachineState::Transition_to_Idle_Loop ||
		ExperimentalState == E_ExperimentalStateMachineState::Transition_to_Locomotion_Loop ||
		ExperimentalState == E_ExperimentalStateMachineState::Transition_to_In_Air_Loop ||
		ExperimentalState == E_ExperimentalStateMachineState::Transition_to_Slide)
	{
		if (StartElapsedTime < 0.4f)
		{
			bShouldLockChooser = true;
		}
	}
	else if (ExperimentalState == E_ExperimentalStateMachineState::Idle_Loop)
	{
		IdleUpdateTimer += DeltaSeconds;
		if (IdleUpdateTimer < 3.0f) bShouldLockChooser = true;
		else IdleUpdateTimer = 0.0f;
	}

	// 3. Trigger Chooser
	if (!bShouldLockChooser)
	{
		UpdateMotionMatchingState();
	}
}

void UGP_MotionMatchingAnimInstance::UpdateLocomotionStates()
{
	const E_Gait PrevGait = Gait;
	const E_MovementState PrevMovementState = MovementState;
	const E_ExperimentalStateMachineState PrevExpState = ExperimentalState;
	const E_MovementMode PrevMovementMode = MovementMode;
	const bool bWasFallingLastFrame = bIsFalling;

	// Physics Data Smoothing
	float TargetSpeed = Velocity.Size2D();
	GroundSpeed = FMath::FInterpTo(GroundSpeed, TargetSpeed, GetWorld()->GetDeltaSeconds(), 15.0f);
	Speed2D = GroundSpeed;

	AbsControlYawDelta = FMath::Abs(ControlYawDelta);
	const EMovementMode NativeMovementMode = MovementComponent->MovementMode;
	const bool bShouldBeInAir = bIsFalling || NativeMovementMode == MOVE_Falling;
	const bool bSprintTagActive = HasSprintTag(Character);
	const bool bIsMovingNow = GroundSpeed > (bWasMovingLastFrame ? 3.0f : 8.0f);
	
	// Stance & Mode
	Stance = bIsCrouching ? E_Stance::Crouch : E_Stance::Stand;
	MovementMode = bShouldBeInAir ? E_MovementMode::InAir : E_MovementMode::OnGround;

	// Rotation Mode
	if (MovementComponent->bOrientRotationToMovement)
	{
		RotationMode = E_RotationMode::VelocityDirection;
	}
	else
	{
		// 임시: 캐릭터에 LockOn 변수가 있다면 LookingDirection으로 설정 가능
		RotationMode = E_RotationMode::LookingDirection;
	}

	// Landing Logic
	JustLanded_Light = false;
	JustLanded_Heavy = false;
	if (bWasFallingLastFrame && !bShouldBeInAir)
	{
		const float LandVelocityZ = FMath::Abs(Velocity.Z);
		if (LandVelocityZ >= HeavyLandSpeedThreshold)
		{
			JustLanded_Heavy = true;
		}
		else
		{
			JustLanded_Light = true;
		}
	}

	// TimeToLand (Falling 시에만 계산)
	if (bShouldBeInAir && Velocity.Z < 0.0f)
	{
		FHitResult Hit;
		FVector Start = Character->GetActorLocation();
		FVector End = Start + (FVector::DownVector * 1000.0f);
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(Character);

		if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
		{
			float Distance = Hit.Distance;
			TimeToLand = Distance / FMath::Abs(Velocity.Z);
		}
		else
		{
			TimeToLand = 99.0f;
		}
	}
	else
	{
		TimeToLand = 0.0f;
	}

	// Gait
	if (Stance == E_Stance::Crouch) Gait = E_Gait::Walk;
	else if (bSprintTagActive || GroundSpeed >= RunSpeedThreshold) Gait = E_Gait::Sprint;
	else if (GroundSpeed >= WalkSpeedThreshold) Gait = E_Gait::Run;
	else Gait = E_Gait::Walk;

	// Movement State
	MovementState = (MovementMode == E_MovementMode::InAir || bIsMovingNow) ? E_MovementState::Moving : E_MovementState::Idle;

	// Helper States
	IsStarting = (MovementMode == E_MovementMode::OnGround) && bIsMovingNow && !bWasMovingLastFrame;
	IsPivoting = (MovementMode == E_MovementMode::OnGround) && (MovementState == E_MovementState::Moving)
		&& GroundSpeed >= PivotMinSpeedThreshold && FMath::Abs(LocalVelocityAngleDegrees) >= PivotMinAngleDegrees;

	// ShouldTurnInPlace
	ShouldTurnInPlace = (MovementState == E_MovementState::Idle) && (AbsControlYawDelta >= TurnInPlaceYawThreshold);

	// ShouldSpinTransition
	if (MovementState == E_MovementState::Moving && !IsStarting && !IsPivoting)
	{
		const FVector AccelDir = Acceleration.GetSafeNormal2D();
		const FVector VelDir = Velocity.GetSafeNormal2D();
		const float AngleDiff = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(AccelDir, VelDir)));
		ShouldSpinTransition = (AngleDiff >= SpinTransitionThreshold);
	}
	else
	{
		ShouldSpinTransition = false;
	}

	// JustTraversed
	if (PrevMovementMode == E_MovementMode::Traversing && MovementMode != E_MovementMode::Traversing)
	{
		JustTraversed = true;
	}
	else if (MovementState == E_MovementState::Moving)
	{
		// 이동 중에는 일정 시간 후 혹은 이동이 안정화되면 해제
		JustTraversed = false;
	}

	// 5. Resolve Experimental State Machine
	if (MovementMode == E_MovementMode::InAir)
	{
		if (PrevMovementMode != E_MovementMode::InAir)
		{
			StartElapsedTime = 0.0f;
			ExperimentalState = E_ExperimentalStateMachineState::Transition_to_In_Air_Loop;
		}
		else if (ExperimentalState == E_ExperimentalStateMachineState::Transition_to_In_Air_Loop && StartElapsedTime >= 0.2f)
		{
			ExperimentalState = E_ExperimentalStateMachineState::In_Air_Loop;
		}
	}
	else if (NativeMovementMode == MOVE_Custom && MovementComponent->CustomMovementMode == 1)
	{
		if (ExperimentalState != E_ExperimentalStateMachineState::Slide_Loop && ExperimentalState != E_ExperimentalStateMachineState::Transition_to_Slide)
		{
			StartElapsedTime = 0.0f;
			ExperimentalState = E_ExperimentalStateMachineState::Transition_to_Slide;
		}
		else if (ExperimentalState == E_ExperimentalStateMachineState::Transition_to_Slide && StartElapsedTime >= 0.3f)
		{
			ExperimentalState = E_ExperimentalStateMachineState::Slide_Loop;
		}
	}
	else
	{
		// Direction
		if (GroundSpeed > 10.0f)
		{
			const float Angle = LocalVelocityAngleDegrees;
			if (Angle >= -45.f && Angle <= 45.f) MovementDirection = E_MovementDirection::Forward;
			else if (Angle > 45.f && Angle <= 135.f) MovementDirection = E_MovementDirection::Right;
			else if (Angle < -45.f && Angle >= -135.f) MovementDirection = E_MovementDirection::Left;
			else MovementDirection = E_MovementDirection::Backward;
		}

		if (MovementState == E_MovementState::Idle)
		{
			if (PrevMovementState == E_MovementState::Moving)
			{
				StartElapsedTime = 0.0f;
				ExperimentalState = E_ExperimentalStateMachineState::Transition_to_Idle_Loop;
			}
			else if (ExperimentalState == E_ExperimentalStateMachineState::Transition_to_Idle_Loop && StartElapsedTime >= 0.45f)
			{
				ExperimentalState = E_ExperimentalStateMachineState::Idle_Loop;
				IdleUpdateTimer = 3.0f; 
			}
			else if (ExperimentalState != E_ExperimentalStateMachineState::Transition_to_Idle_Loop)
			{
				ExperimentalState = E_ExperimentalStateMachineState::Idle_Loop;
			}
		}
		else // Moving
		{
			const FVector VelocityNormal = Velocity.GetSafeNormal2D();
			const FVector AccelNormal = Acceleration.GetSafeNormal2D();
			float AccelDotVel = FVector::DotProduct(VelocityNormal, AccelNormal);
			bool bIsCorrecting = (AccelDotVel < -0.1f) && (GroundSpeed > 150.0f);

			bool bIsAlreadyTransitioning = (ExperimentalState == E_ExperimentalStateMachineState::Transition_to_Locomotion_Loop && StartElapsedTime < 0.4f);

			if (!bIsAlreadyTransitioning && (PrevMovementState == E_MovementState::Idle || IsStarting || IsPivoting || bIsCorrecting))
			{
				StartElapsedTime = 0.0f;
				ExperimentalState = E_ExperimentalStateMachineState::Transition_to_Locomotion_Loop;
			}
			else if (ExperimentalState == E_ExperimentalStateMachineState::Transition_to_Locomotion_Loop && StartElapsedTime >= 0.4f)
			{
				ExperimentalState = E_ExperimentalStateMachineState::Locomotion_Loop;
			}
			else if (ExperimentalState != E_ExperimentalStateMachineState::Transition_to_Locomotion_Loop)
			{
				ExperimentalState = E_ExperimentalStateMachineState::Locomotion_Loop;
			}
		}
	}

	bWasMovingLastFrame = bIsMovingNow;

	if (bDebugMotionMatchingState && (PrevExpState != ExperimentalState))
	{
		FString StateName;
		const UEnum* EnumPtr = StaticEnum<E_ExperimentalStateMachineState>();
		if (EnumPtr) StateName = EnumPtr->GetNameStringByValue((int64)ExperimentalState);
		UE_LOG(LogTemp, Warning, TEXT(">> [ExpState] %s (Speed: %.1f)"), *StateName, GroundSpeed);
	}
}

void UGP_MotionMatchingAnimInstance::SetSelectedPoseSearchDatabase(UPoseSearchDatabase* NewDatabase)
{
	if (SelectedPoseSearchDatabase != NewDatabase)
	{
		SelectedPoseSearchDatabase = NewDatabase;
		if (bDebugMotionMatchingState && NewDatabase)
		{
			UE_LOG(LogTemp, Log, TEXT(">> [Chooser] Base PSD Changed: %s"), *NewDatabase->GetName());
		}
	}
}

void UGP_MotionMatchingAnimInstance::SetSelectedTransitionDatabase(UPoseSearchDatabase* NewDatabase)
{
	if (SelectedTransitionDatabase != NewDatabase)
	{
		SelectedTransitionDatabase = NewDatabase;
		if (bDebugMotionMatchingState && NewDatabase)
		{
			UE_LOG(LogTemp, Warning, TEXT(">> [Chooser] Transition PSD Changed: %s"), *NewDatabase->GetName());
		}
	}

	// SAFETY FALLBACK: 만약 전이 DB가 없으면 일반 루프 DB라도 사용하도록 강제
	if (SelectedTransitionDatabase == nullptr && SelectedPoseSearchDatabase != nullptr)
	{
		SelectedTransitionDatabase = SelectedPoseSearchDatabase;
	}
}

void UGP_MotionMatchingAnimInstance::UpdateMotionMatchingState_Implementation()
{
}
