#include "Animation/GP_CharacterAnimInstance.h"
#include "CharacterTrajectoryComponent.h"
#include "ChooserFunctionLibrary.h"
#include "Engine/Engine.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PoseSearch/PoseSearchDatabase.h"
#include "UObject/UnrealType.h"

namespace
{
const TCHAR* DefaultPoseSearchChooserPath = TEXT("/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/ChooserTables/CHT_MM_MaskMan_Root.CHT_MM_MaskMan_Root");

E_MovementDirection ResolveMovementDirection(const FVector& LocalVelocityDirection)
{
	const FVector2D Direction2D(LocalVelocityDirection.X, LocalVelocityDirection.Y);
	if (Direction2D.IsNearlyZero())
	{
		return E_MovementDirection::Forward;
	}

	const bool bForward = Direction2D.X >= 0.f;
	const bool bRight = Direction2D.Y >= 0.f;
	const float AbsX = FMath::Abs(Direction2D.X);
	const float AbsY = FMath::Abs(Direction2D.Y);

	if (AbsX >= AbsY * 1.5f)
	{
		return bForward ? E_MovementDirection::Forward : E_MovementDirection::Backward;
	}

	if (AbsY >= AbsX * 1.5f)
	{
		return bRight ? E_MovementDirection::Right : E_MovementDirection::Left;
	}

	if (AbsX >= AbsY)
	{
		return bForward ? E_MovementDirection::Forward : E_MovementDirection::Backward;
	}

	return bRight ? E_MovementDirection::Right : E_MovementDirection::Left;
}

E_Gait ResolveGait(bool bIsMoving, bool bIsSprinting)
{
	if (!bIsMoving)
	{
		return E_Gait::Walk;
	}

	if (bIsSprinting)
	{
		return E_Gait::Sprint;
	}

	return E_Gait::Run;
}

ESourceMotionMatchState ResolveMotionMatchState(E_MovementMode MovementMode, E_MovementState MovementState, E_Gait Gait)
{
	if (MovementMode == E_MovementMode::InAir)
	{
		return ESourceMotionMatchState::Jump;
	}

	if (MovementState == E_MovementState::Idle)
	{
		return ESourceMotionMatchState::Idle;
	}

	if (Gait == E_Gait::Sprint)
	{
		return ESourceMotionMatchState::Sprint;
	}

	if (Gait == E_Gait::Run)
	{
		return ESourceMotionMatchState::Run;
	}

	return ESourceMotionMatchState::Walk;
}
}

UGP_CharacterAnimInstance::UGP_CharacterAnimInstance()
{
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

	if (!PoseSearchChooser)
	{
		PoseSearchChooser = LoadObject<UChooserTable>(nullptr, DefaultPoseSearchChooserPath);
	}
}

void UGP_CharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!Character || !MovementComponent)
	{
		Character = Cast<ACharacter>(TryGetPawnOwner());
		if (Character)
		{
			MovementComponent = Character->GetCharacterMovement();
		}
	}

	if (AnimationSet)
	{
		LocomotionBlendSpace = AnimationSet->LocomotionBlendSpace;
		JumpLoopAnimation = AnimationSet->JumpLoopAnimation;
		SprintStopAnimation = AnimationSet->SprintStopAnimation;
	}

	if (!PoseSearchChooser)
	{
		PoseSearchChooser = LoadObject<UChooserTable>(nullptr, DefaultPoseSearchChooserPath);
	}

	if (!Character || !MovementComponent)
	{
		return;
	}

	const FVector WorldVelocity = Character->GetVelocity();
	GroundSpeed = WorldVelocity.Size2D();
	Speed2D = GroundSpeed;
	LocalVelocityDirection = Character->GetActorTransform().InverseTransformVectorNoScale(WorldVelocity).GetSafeNormal2D();

	bHasAcceleration = MovementComponent->GetCurrentAcceleration().SizeSquared2D() > KINDA_SMALL_NUMBER;
	bIsFalling = MovementComponent->IsFalling();
	bIsAnyMontagePlaying = Montage_IsPlaying(nullptr);
	const float CurrentMaxSpeed = MovementComponent->GetMaxSpeed();
	bIsSprinting = CurrentMaxSpeed >= SprintSpeedThreshold || GroundSpeed >= SprintSpeedThreshold;
	const bool bIsMovingNow = GroundSpeed > IdleSpeedThreshold || bHasAcceleration;
	const bool bWasFallingLastFrame = MovementMode == E_MovementMode::InAir;
	const E_MovementDirection PreviousDirection = MovementDirection;

	if (bIsMovingNow)
	{
		TimeSinceMovementStarted = bWasMovingLastFrame ? (TimeSinceMovementStarted + DeltaSeconds) : 0.f;
	}
	else
	{
		TimeSinceMovementStarted = 0.f;
	}

	const bool bRecentlyStartedMoving = bIsMovingNow && TimeSinceMovementStarted < MovementStartGraceTime;

	MovementMode_LastFrame = MovementMode;
	Gait_LastFrame = Gait;

	MovementMode = bIsFalling ? E_MovementMode::InAir : E_MovementMode::Grounded;
	Stance = Character->bIsCrouched ? E_Stance::Crouching : E_Stance::Standing;
	MovementState = bIsMovingNow ? E_MovementState::Moving : E_MovementState::Idle;
	Gait = ResolveGait(bIsMovingNow, bIsSprinting);
	MovementDirection = ResolveMovementDirection(LocalVelocityDirection);

	// Detect movement start for Chooser to select Start animations
	IsStarting = !bWasMovingLastFrame && bIsMovingNow && bHasAcceleration;

	const FVector LastDirection = LastLocalVelocityDirection.GetSafeNormal2D();
	const FVector CurrentDirection = LocalVelocityDirection.GetSafeNormal2D();
	const float DirectionDot = FVector::DotProduct(LastDirection, CurrentDirection);
	IsPivoting = bIsMovingNow && !LastDirection.IsNearlyZero() && DirectionDot < -0.35f;

	const float CurrentYaw = Character->GetActorRotation().Yaw;
	const float DesiredYaw = Character->GetControlRotation().Yaw;
	const float YawDelta = FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentYaw, DesiredYaw));
	ShouldTurnInPlace = !bIsMovingNow && !bIsFalling && YawDelta > 50.f;
	ShouldSpinTransition = false;

	JustTraversed = false;
	JustLanded_Light = bWasFallingLastFrame && !bIsFalling && FMath::Abs(LastVerticalVelocity) < 900.f;
	JustLanded_Heavy = bWasFallingLastFrame && !bIsFalling && FMath::Abs(LastVerticalVelocity) >= 900.f;

	MovementDirection_Recent = static_cast<uint8>(MovementDirection);
	if (PreviousDirection != MovementDirection)
	{
		MovementDirection_Recent |= static_cast<uint8>(PreviousDirection);
	}

	bool bUsedBlueprintTrajectory = false;
	if (const FObjectPropertyBase* CharacterTrajectoryProperty = FindFProperty<FObjectPropertyBase>(Character->GetClass(), TEXT("CharacterTrajectory")))
	{
		if (UCharacterTrajectoryComponent* CharacterTrajectoryComponent = Cast<UCharacterTrajectoryComponent>(CharacterTrajectoryProperty->GetObjectPropertyValue_InContainer(Character)))
		{
			if (const FStructProperty* TrajectoryProperty = FindFProperty<FStructProperty>(UCharacterTrajectoryComponent::StaticClass(), TEXT("Trajectory")))
			{
				if (const FTransformTrajectory* ComponentTrajectory = TrajectoryProperty->ContainerPtrToValuePtr<FTransformTrajectory>(CharacterTrajectoryComponent))
				{
					GeneratedTrajectory = *ComponentTrajectory;
					bUsedBlueprintTrajectory = true;
				}
			}
		}
	}

	if (!bUsedBlueprintTrajectory)
	{
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
	}

	if (PoseSearchChooser)
	{
		if (UPoseSearchDatabase* SelectedDatabase = Cast<UPoseSearchDatabase>(
			UChooserFunctionLibrary::EvaluateChooser(this, PoseSearchChooser, UPoseSearchDatabase::StaticClass())))
		{
			ApplyChosenDatabase(SelectedDatabase);
		}
	}
	else if (bIsFalling && JumpPoseSearchDatabase)
	{
		CurrentMotionMatchState = ESourceMotionMatchState::Jump;
		RuntimePoseSearchDatabase = JumpPoseSearchDatabase;
	}
	else if (bIsSprinting && bIsMovingNow && SprintPoseSearchDatabase)
	{
		CurrentMotionMatchState = ESourceMotionMatchState::Sprint;
		RuntimePoseSearchDatabase = SprintPoseSearchDatabase;
	}
	else if (!bIsMovingNow && IdlePoseSearchDatabase)
	{
		CurrentMotionMatchState = ESourceMotionMatchState::Idle;
		RuntimePoseSearchDatabase = IdlePoseSearchDatabase;
	}
	else if ((GroundSpeed > WalkSpeedThreshold || bRecentlyStartedMoving) && RunPoseSearchDatabase)
	{
		CurrentMotionMatchState = ESourceMotionMatchState::Run;
		RuntimePoseSearchDatabase = RunPoseSearchDatabase;
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

	bWasMovingLastFrame = bIsMovingNow;
	LastLocalVelocityDirection = LocalVelocityDirection;
	LastVerticalVelocity = WorldVelocity.Z;

#if !UE_BUILD_SHIPPING
	if (GEngine)
	{
		const FString DebugText = FString::Printf(
			TEXT("MM Speed: %.1f | Gait: %s | State: %s | DB: %s"),
			GroundSpeed,
			*UEnum::GetValueAsString(Gait),
			*UEnum::GetValueAsString(CurrentMotionMatchState),
			*GetNameSafe(RuntimePoseSearchDatabase));
		GEngine->AddOnScreenDebugMessage(
			0x4D4D4442,
			0.f,
			FColor::Cyan,
			DebugText);
	}
#endif
}

void UGP_CharacterAnimInstance::ApplyChosenDatabase(UPoseSearchDatabase* SelectedDatabase)
{
	if (!SelectedDatabase)
	{
		return;
	}

	RuntimePoseSearchDatabase = SelectedDatabase;
	CurrentMotionMatchState = ResolveMotionMatchState(MovementMode, MovementState, Gait);

	const FString DatabaseName = SelectedDatabase->GetName();
	if (DatabaseName.Contains(TEXT("Sprint")))
	{
		SprintPoseSearchDatabase = SelectedDatabase;
	}
	else if (DatabaseName.Contains(TEXT("Run")))
	{
		RunPoseSearchDatabase = SelectedDatabase;
	}
	else if (DatabaseName.Contains(TEXT("Walk")))
	{
		WalkPoseSearchDatabase = SelectedDatabase;
	}
	else if (DatabaseName.Contains(TEXT("Jump")) || DatabaseName.Contains(TEXT("Land")))
	{
		JumpPoseSearchDatabase = SelectedDatabase;
	}
}

void UGP_CharacterAnimInstance::SetAnimationSet(UPDA_CharacterAnimationSet* NewSet)
{
	if (NewSet)
	{
		AnimationSet = NewSet;
		LocomotionBlendSpace = AnimationSet->LocomotionBlendSpace;
		JumpLoopAnimation = AnimationSet->JumpLoopAnimation;
		SprintStopAnimation = AnimationSet->SprintStopAnimation;
	}
}

void UGP_CharacterAnimInstance::ApplyRuntimeDatabaseToMotionMatchingNode(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	EAnimNodeReferenceConversionResult ConversionResult = EAnimNodeReferenceConversionResult::Failed;
	const FMotionMatchingAnimNodeReference MotionMatchingNode =
		UMotionMatchingAnimNodeLibrary::ConvertToMotionMatchingNode(Node, ConversionResult);

	if (ConversionResult != EAnimNodeReferenceConversionResult::Succeeded)
	{
		return;
	}

	if (RuntimePoseSearchDatabase)
	{
		// ForceInterrupt causes popping. Use DoNotInterrupt for smooth transitions 
		// unless we are specifically handling a hard state change.
		const EPoseSearchInterruptMode InterruptMode = EPoseSearchInterruptMode::DoNotInterrupt;

		UMotionMatchingAnimNodeLibrary::SetDatabaseToSearch(
			MotionMatchingNode,
			RuntimePoseSearchDatabase,
			InterruptMode);

		LastAppliedRuntimePoseSearchDatabase = RuntimePoseSearchDatabase;
	}
	else
	{
		UMotionMatchingAnimNodeLibrary::ResetDatabasesToSearch(
			MotionMatchingNode,
			EPoseSearchInterruptMode::DoNotInterrupt);

		LastAppliedRuntimePoseSearchDatabase = nullptr;
	}

	FPoseSearchBlueprintResult SearchResult;
	bool bIsResultValid = false;
	UMotionMatchingAnimNodeLibrary::GetMotionMatchingSearchResult(MotionMatchingNode, SearchResult, bIsResultValid);
	bMotionMatchingResultValid = bIsResultValid;
	MotionMatchingSelectedAnimName =
		(bIsResultValid && SearchResult.SelectedAnim)
			? SearchResult.SelectedAnim->GetFName()
			: NAME_None;
}
