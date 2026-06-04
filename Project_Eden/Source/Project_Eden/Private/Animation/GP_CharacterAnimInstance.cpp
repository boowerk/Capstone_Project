#include "Animation/GP_CharacterAnimInstance.h"
#include "CharacterTrajectoryComponent.h"
#include "Characters/GP_PlayerCharacter.h"
#include "ChooserFunctionLibrary.h"
#include "Engine/Engine.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PoseSearch/PoseSearchDatabase.h"
#include "UObject/UnrealType.h"

namespace
{
const TCHAR* DefaultPoseSearchChooserPath = TEXT("/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/ChooserTables/CHT_MM_MaskMan_Root_OriginalStyle.CHT_MM_MaskMan_Root_OriginalStyle");
const TCHAR* DeprecatedPoseSearchChooserName = TEXT("CHT_MM_MaskMan_Root.");

void OverrideTrajectoryPrediction(FTransformTrajectory& Trajectory, const FVector& WorldVelocity)
{
	if (Trajectory.Samples.IsEmpty() || WorldVelocity.IsNearlyZero())
	{
		return;
	}

	int32 CurrentSampleIndex = 0;
	float ClosestTimeToCurrent = TNumericLimits<float>::Max();
	for (int32 SampleIndex = 0; SampleIndex < Trajectory.Samples.Num(); ++SampleIndex)
	{
		const float TimeDistance = FMath::Abs(Trajectory.Samples[SampleIndex].TimeInSeconds);
		if (TimeDistance < ClosestTimeToCurrent)
		{
			ClosestTimeToCurrent = TimeDistance;
			CurrentSampleIndex = SampleIndex;
		}
	}

	const FVector CurrentPosition = Trajectory.Samples[CurrentSampleIndex].Position;
	for (FTransformTrajectorySample& Sample : Trajectory.Samples)
	{
		if (Sample.TimeInSeconds > 0.0f)
		{
			Sample.Position = CurrentPosition + (WorldVelocity * Sample.TimeInSeconds);
		}
	}
}

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

void NormalizeTrajectoryForMovementScale(FTransformTrajectory& Trajectory, float MovementScaleRatio)
{
	if (FMath::IsNearlyEqual(MovementScaleRatio, 1.f) || Trajectory.Samples.IsEmpty())
	{
		return;
	}

	const float SafeScaleRatio = FMath::Max(MovementScaleRatio, 0.01f);
	int32 CurrentSampleIndex = 0;
	float ClosestTimeToCurrent = TNumericLimits<float>::Max();
	for (int32 SampleIndex = 0; SampleIndex < Trajectory.Samples.Num(); ++SampleIndex)
	{
		const float TimeDistance = FMath::Abs(Trajectory.Samples[SampleIndex].TimeInSeconds);
		if (TimeDistance < ClosestTimeToCurrent)
		{
			ClosestTimeToCurrent = TimeDistance;
			CurrentSampleIndex = SampleIndex;
		}
	}

	const FVector CurrentPosition = Trajectory.Samples[CurrentSampleIndex].Position;
	for (FTransformTrajectorySample& Sample : Trajectory.Samples)
	{
		Sample.Position = CurrentPosition + ((Sample.Position - CurrentPosition) / SafeScaleRatio);
	}
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
	if (!Character)
	{
		Character = Cast<ACharacter>(GetOwningActor());
	}
	if (Character)
	{
		MovementComponent = Character->GetCharacterMovement();
	}

	const AGP_PlayerCharacter* PlayerCharacter = Cast<AGP_PlayerCharacter>(Character);
	if (PlayerCharacter && PlayerCharacter->GetUEFNSourceAnimInstance() == this &&
		(!PoseSearchChooser || PoseSearchChooser->GetPathName().Contains(DeprecatedPoseSearchChooserName)))
	{
		PoseSearchChooser = LoadObject<UChooserTable>(nullptr, DefaultPoseSearchChooserPath);
	}
}

void UGP_CharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (MotionMatchingSuppressTimeRemaining > 0.f)
	{
		MotionMatchingSuppressTimeRemaining = FMath::Max(0.f, MotionMatchingSuppressTimeRemaining - DeltaSeconds);
	}

	if (!Character || !MovementComponent)
	{
		Character = Cast<ACharacter>(TryGetPawnOwner());
		if (!Character)
		{
			Character = Cast<ACharacter>(GetOwningActor());
		}
		if (Character)
		{
			MovementComponent = Character->GetCharacterMovement();
		}
	}

	if (!Character || !MovementComponent)
	{
		return;
	}

	FVector WorldVelocity = Character->GetVelocity();
	const FVector WorldAcceleration = MovementComponent->GetCurrentAcceleration();
	const AGP_PlayerCharacter* PlayerCharacter = Cast<AGP_PlayerCharacter>(Character);
	const bool bCanUseRuntimePoseSearchChooser = PlayerCharacter && PlayerCharacter->GetUEFNSourceAnimInstance() == this;
	const bool bSourceFallbackMontageActive = PlayerCharacter && PlayerCharacter->IsPlayingUEFNSourceFallbackMontage();
	bool bBlendActionLowerBodyToMM = false;
	if (PlayerCharacter && PlayerCharacter->ShouldBlendActionLowerBodyToMotionMatching())
	{
		bBlendActionLowerBodyToMM = bCanUseRuntimePoseSearchChooser
			? bSourceFallbackMontageActive
			: !bSourceFallbackMontageActive;
	}
	const float LowerBodyBlendSpeed = bBlendActionLowerBodyToMM ? ActionLowerBodyMotionMatchBlendInSpeed : ActionLowerBodyMotionMatchBlendOutSpeed;
	ActionLowerBodyMotionMatchBlendAlpha = FMath::FInterpTo(
		ActionLowerBodyMotionMatchBlendAlpha,
		bBlendActionLowerBodyToMM ? 1.f : 0.f,
		DeltaSeconds,
		LowerBodyBlendSpeed);

	FVector ActionMotionVelocity = FVector::ZeroVector;
	if (bCanUseRuntimePoseSearchChooser)
	{
		ActionMotionVelocity = PlayerCharacter->GetActionMotionAnimVelocity();
	}
	if (bCanUseRuntimePoseSearchChooser && (PlayerCharacter->IsPlayingUEFNSourceFallbackMontage() || !ActionMotionVelocity.IsNearlyZero()))
	{
		if (!ActionMotionVelocity.IsNearlyZero())
		{
			WorldVelocity.X = ActionMotionVelocity.X;
			WorldVelocity.Y = ActionMotionVelocity.Y;
		}
	}
	if (bCanUseRuntimePoseSearchChooser &&
		(!PoseSearchChooser || PoseSearchChooser->GetPathName().Contains(DeprecatedPoseSearchChooserName)))
	{
		PoseSearchChooser = LoadObject<UChooserTable>(nullptr, DefaultPoseSearchChooserPath);
	}
	MovementSpeedScaleRatio = PlayerCharacter ? PlayerCharacter->GetMovementSpeedScaleRatio() : 1.0f;
	GroundSpeed = WorldVelocity.Size2D();
	Speed2D = GroundSpeed / MovementSpeedScaleRatio;
	LocalVelocityDirection = Character->GetActorTransform().InverseTransformVectorNoScale(WorldVelocity).GetSafeNormal2D();
	const FVector LocalAccelerationDirection =
		Character->GetActorTransform().InverseTransformVectorNoScale(WorldAcceleration).GetSafeNormal2D();

	bHasAcceleration = WorldAcceleration.SizeSquared2D() > KINDA_SMALL_NUMBER;
	bIsFalling = MovementComponent->IsFalling();
	bIsAnyMontagePlaying = Montage_IsPlaying(nullptr);
	const float CurrentMaxSpeed = MovementComponent->GetMaxSpeed() / MovementSpeedScaleRatio;
	bIsSprinting = CurrentMaxSpeed >= SprintSpeedThreshold || Speed2D >= SprintSpeedThreshold;
	const bool bWantsGroundMovement = bHasAcceleration && !bIsFalling;
	const bool bIsMovingNow = Speed2D > IdleSpeedThreshold || bWantsGroundMovement;
	const bool bWasFallingLastFrame = MovementMode == E_MovementMode::InAir;
	const E_MovementDirection PreviousDirection = MovementDirection;

	if (bIsMovingNow)
	{
		TimeSinceMovementStarted = bWasMovingLastFrame ? (TimeSinceMovementStarted + DeltaSeconds) : 0.f;
		TimeSinceMovementStopped = 0.f;
	}
	else
	{
		TimeSinceMovementStarted = 0.f;
		TimeSinceMovementStopped = bWasMovingLastFrame ? 0.f : (TimeSinceMovementStopped + DeltaSeconds);
	}

	TimeSinceLastLanded += DeltaSeconds;
	TimeSinceStopStarted += DeltaSeconds;
	TimeSincePivotStarted += DeltaSeconds;

	const bool bRecentlyStartedMoving = bIsMovingNow && TimeSinceMovementStarted < MovementStartGraceTime;

	MovementMode_LastFrame = MovementMode;
	Gait_LastFrame = Gait;

	MovementMode = bIsFalling ? E_MovementMode::InAir : E_MovementMode::Grounded;
	Stance = Character->bIsCrouched ? E_Stance::Crouching : E_Stance::Standing;
	MovementState = bIsMovingNow ? E_MovementState::Moving : E_MovementState::Idle;
	Gait = ResolveGait(bIsMovingNow, bIsSprinting);
	MovementDirection = ResolveMovementDirection(LocalVelocityDirection);
	if (bIsFalling && FMath::Abs(GetWorld()->GetGravityZ()) > UE_SMALL_NUMBER)
	{
		TimeToLand = FMath::Max(0.f, -Character->GetVelocity().Z / FMath::Abs(GetWorld()->GetGravityZ()));
	}
	else
	{
		TimeToLand = 0.f;
	}

	// Detect movement start for Chooser to select Start animations
	IsStarting = !bWasMovingLastFrame && bWantsGroundMovement;
	const bool bStopTriggeredThisFrame = bWasMovingLastFrame && !bHasAcceleration && !bIsFalling;
	if (bStopTriggeredThisFrame)
	{
		TimeSinceStopStarted = 0.f;
	}

	const bool bHoldStopState = !bHasAcceleration && !bIsFalling && TimeSinceStopStarted <= StopHoldDuration;
	IsStopping = bStopTriggeredThisFrame || bHoldStopState;

	const float CurrentYaw = Character->GetActorRotation().Yaw;
	const float DesiredYaw = Character->GetControlRotation().Yaw;
	const float YawDelta = FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentYaw, DesiredYaw));
	const float YawRate = DeltaSeconds > UE_SMALL_NUMBER
		? FMath::Abs(FMath::FindDeltaAngleDegrees(LastActorYaw, CurrentYaw)) / DeltaSeconds
		: 0.f;
	const float PivotDirectionDot =
		(!LocalVelocityDirection.IsNearlyZero() && !LocalAccelerationDirection.IsNearlyZero())
			? FVector::DotProduct(LocalVelocityDirection, LocalAccelerationDirection)
			: 1.f;
	const bool bPivotTriggeredThisFrame =
		bIsMovingNow &&
		bHasAcceleration &&
		Speed2D >= WalkSpeedThreshold &&
		PivotDirectionDot < PivotDirectionDotThreshold;
	if (bPivotTriggeredThisFrame)
	{
		TimeSincePivotStarted = 0.f;
	}

	const bool bCanContinuePivot =
		bIsMovingNow &&
		bHasAcceleration &&
		Speed2D >= WalkSpeedThreshold &&
		PivotDirectionDot < PivotExitDirectionDotThreshold;

	const bool bHoldPivotState =
		bCanContinuePivot &&
		TimeSincePivotStarted <= PivotHoldDuration;

	IsPivoting = bPivotTriggeredThisFrame || (IsPivoting && bCanContinuePivot) || bHoldPivotState;

	// Update Turn In Place elapsed time
	if (ShouldTurnInPlace)
	{
		TimeSinceTurnInPlaceStarted += DeltaSeconds;
	}
	else
	{
		TimeSinceTurnInPlaceStarted = 0.f;
	}

	const float ActiveTurnThreshold = ShouldTurnInPlace ? 2.0f : TurnInPlaceYawThreshold;
	const bool bCanKeepTurnInPlace = !bIsFalling && Speed2D <= TurnInPlaceMaxSpeed && !bHasAcceleration;

	bool bWantTurnInPlace =
		bCanKeepTurnInPlace &&
		TimeSinceMovementStopped >= TurnInPlaceMinIdleTime &&
		TimeSinceLastLanded > LandedSignalDuration &&
		YawDelta > ActiveTurnThreshold;

	// Hysteresis: Keep TurnInPlace active for a minimum duration to prevent animation chattering
	if (ShouldTurnInPlace && bCanKeepTurnInPlace && TimeSinceTurnInPlaceStarted < TurnInPlaceMinDuration)
	{
		bWantTurnInPlace = true;
	}

	// Lock: If motion matching is currently selecting a turn animation, keep TurnInPlace active
	// so the animation doesn't get cut off in the middle of a rotation.
	if (ShouldTurnInPlace && !MotionMatchingSelectedAnimName.IsNone())
	{
		const FString AnimNameStr = MotionMatchingSelectedAnimName.ToString();
		if (AnimNameStr.Contains(TEXT("Turn")) || AnimNameStr.Contains(TEXT("turn")) || AnimNameStr.Contains(TEXT("TIP")))
		{
			// Reaction: Allow instant interruption if the player actually starts moving
			if (bCanKeepTurnInPlace)
			{
				bWantTurnInPlace = true;
			}
		}
	}

	ShouldTurnInPlace = bWantTurnInPlace;
	ShouldSpinTransition =
		bIsMovingNow &&
		bHasAcceleration &&
		Speed2D >= RunSpeedThreshold &&
		!IsStopping &&
		!IsPivoting &&
		!ShouldTurnInPlace &&
		YawRate >= MovingTurnYawRateThreshold;

	JustTraversed = false;
	if (bWasFallingLastFrame && !bIsFalling)
	{
		TimeSinceLastLanded = 0.f;
	}

	const bool bLandedRecently = TimeSinceLastLanded <= LandedSignalDuration;
	JustLanded_Light = bLandedRecently && FMath::Abs(LastVerticalVelocity) < 900.f;
	JustLanded_Heavy = bLandedRecently && FMath::Abs(LastVerticalVelocity) >= 900.f;

	MovementDirection_Recent = static_cast<uint8>(MovementDirection);
	if (PreviousDirection != MovementDirection)
	{
		MovementDirection_Recent |= static_cast<uint8>(PreviousDirection);
	}

	// Update DesiredControllerYawLastUpdate to actual camera desired yaw for MM trajectory prediction
	DesiredControllerYawLastUpdate = DesiredYaw;

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
					NormalizeTrajectoryForMovementScale(GeneratedTrajectory, MovementSpeedScaleRatio);
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
		NormalizeTrajectoryForMovementScale(GeneratedTrajectory, MovementSpeedScaleRatio);
	}
	if (bCanUseRuntimePoseSearchChooser && PlayerCharacter->IsUsingPostActionAnimVelocity())
	{
		FVector NormalizedActionMotionVelocity = ActionMotionVelocity;
		NormalizedActionMotionVelocity.Z = 0.0f;
		OverrideTrajectoryPrediction(GeneratedTrajectory, NormalizedActionMotionVelocity / MovementSpeedScaleRatio);
		UE_LOG(LogTemp, Warning, TEXT("[ActionRM][AnimTrajectory] Mesh=%s Speed=%.1f Samples=%d"),
			*GetNameSafe(GetOwningComponent()),
			NormalizedActionMotionVelocity.Size2D(),
			GeneratedTrajectory.Samples.Num());
	}

	if (bCanUseRuntimePoseSearchChooser && PoseSearchChooser)
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
	else if ((Speed2D > WalkSpeedThreshold || bRecentlyStartedMoving) && RunPoseSearchDatabase)
	{
		CurrentMotionMatchState = ESourceMotionMatchState::Run;
		RuntimePoseSearchDatabase = RunPoseSearchDatabase;
	}
	else if (Speed2D <= WalkSpeedThreshold && WalkPoseSearchDatabase)
	{
		CurrentMotionMatchState = ESourceMotionMatchState::Walk;
		RuntimePoseSearchDatabase = WalkPoseSearchDatabase;
	}
	else if (Speed2D <= RunSpeedThreshold && RunPoseSearchDatabase)
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
	LastActorYaw = CurrentYaw;

#if !UE_BUILD_SHIPPING
	const bool bIsUEFNSourceAnimInstance = PlayerCharacter && PlayerCharacter->GetUEFNSourceAnimInstance() == this;
	if (GEngine && PlayerCharacter && PlayerCharacter->IsLocallyControlled() && bIsUEFNSourceAnimInstance)
	{
		const FString SelectedAnimText = bMotionMatchingResultValid
			? (MotionMatchingSelectedAnimName.IsNone() ? TEXT("Null Asset") : MotionMatchingSelectedAnimName.ToString())
			: TEXT("Invalid Result");
		const FString DebugText = FString::Printf(
			TEXT("DBG v3 Mesh: %s (UEFNSource) \nMM Speed2D: %.1f \nRaw Speed: %.1f \nSpeed Scale: %.2f \nProfile Scale: %.2f \nMaxWalk: %.1f \nGait: %s \nState: %s \nTurn:%d \nStop:%d \nPivot:%d \nSpin:%d \nPivotDot: %.2f \nYawRate: %.1f \nIdleT: %.2f \nStopT: %.2f \nPivotT: %.2f \nLandT: %.2f"),
			*GetNameSafe(GetSkelMeshComponent()),
			Speed2D,
			GroundSpeed,
			MovementSpeedScaleRatio,
			PlayerCharacter ? PlayerCharacter->GetActiveMovementSpeedProfile().MovementSpeedScaleRatio : 1.0f,
			MovementComponent ? MovementComponent->MaxWalkSpeed : 0.0f,
			*UEnum::GetValueAsString(Gait),
			*UEnum::GetValueAsString(CurrentMotionMatchState),
			ShouldTurnInPlace ? 1 : 0,
			IsStopping ? 1 : 0,
			IsPivoting ? 1 : 0,
			ShouldSpinTransition ? 1 : 0,
			PivotDirectionDot,
			YawRate,
			TimeSinceMovementStopped,
			TimeSinceStopStarted,
			TimeSincePivotStarted,
			TimeSinceLastLanded);
		const FString DebugAssetText = FString::Printf(
			TEXT("MM DB: %s \nApplied DB: %s \nValid:%d \nAnim: %s"),
			*GetNameSafe(RuntimePoseSearchDatabase),
			*GetNameSafe(LastAppliedRuntimePoseSearchDatabase),
			bMotionMatchingResultValid ? 1 : 0,
			*SelectedAnimText);
		GEngine->AddOnScreenDebugMessage(
			0x4D4D4452,
			0.f,
			FColor::Cyan,
			DebugText);
		GEngine->AddOnScreenDebugMessage(
			0x4D4D4453,
			0.f,
			FColor::Green,
			DebugAssetText);
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

	const FString DatabaseName = SelectedDatabase->GetName();
	if (DatabaseName.Contains(TEXT("Sprint")))
	{
		CurrentMotionMatchState = ESourceMotionMatchState::Sprint;
		SprintPoseSearchDatabase = SelectedDatabase;
	}
	else if (DatabaseName.Contains(TEXT("Run")))
	{
		CurrentMotionMatchState = ESourceMotionMatchState::Run;
		RunPoseSearchDatabase = SelectedDatabase;
	}
	else if (DatabaseName.Contains(TEXT("Walk")))
	{
		CurrentMotionMatchState = ESourceMotionMatchState::Walk;
		WalkPoseSearchDatabase = SelectedDatabase;
	}
	else if (DatabaseName.Contains(TEXT("Jump")) || DatabaseName.Contains(TEXT("Land")))
	{
		CurrentMotionMatchState = ESourceMotionMatchState::Jump;
		JumpPoseSearchDatabase = SelectedDatabase;
	}
	else
	{
		CurrentMotionMatchState = ESourceMotionMatchState::Idle;
		IdlePoseSearchDatabase = SelectedDatabase;
	}
}

void UGP_CharacterAnimInstance::SetAnimationSet(UPDA_CharacterAnimationSet* NewSet)
{
	if (NewSet)
	{
		AnimationSet = NewSet;
	}
}

void UGP_CharacterAnimInstance::SetMovementSpeedScaleRatio(float NewRatio)
{
	MovementSpeedScaleRatio = FMath::Max(NewRatio, 0.01f);
}

void UGP_CharacterAnimInstance::SuppressMotionMatchingUpdate(float Duration)
{
	MotionMatchingSuppressTimeRemaining = FMath::Max(MotionMatchingSuppressTimeRemaining, Duration);
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

	if (MotionMatchingSuppressTimeRemaining > 0.f)
	{
		UMotionMatchingAnimNodeLibrary::ResetDatabasesToSearch(
			MotionMatchingNode,
			EPoseSearchInterruptMode::DoNotInterrupt);

		LastAppliedRuntimePoseSearchDatabase = nullptr;
		bMotionMatchingResultValid = false;
		MotionMatchingSelectedAnimName = NAME_None;
		return;
	}

	if (RuntimePoseSearchDatabase)
	{
		const bool bRuntimeDatabaseChanged = RuntimePoseSearchDatabase != LastAppliedRuntimePoseSearchDatabase;
		const bool bEnteringJumpDatabase = CurrentMotionMatchState == ESourceMotionMatchState::Jump && bRuntimeDatabaseChanged;
		const EPoseSearchInterruptMode InterruptMode = bEnteringJumpDatabase
			? EPoseSearchInterruptMode::ForceInterruptAndInvalidateContinuingPose
			: EPoseSearchInterruptMode::DoNotInterrupt;

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
	
	// SelectedAnim이 유효한지 체크 강화
	MotionMatchingSelectedAnimName = NAME_None;
	if (bIsResultValid)
	{
		if (SearchResult.SelectedAnim)
		{
			MotionMatchingSelectedAnimName = SearchResult.SelectedAnim->GetFName();
		}
	}
}
