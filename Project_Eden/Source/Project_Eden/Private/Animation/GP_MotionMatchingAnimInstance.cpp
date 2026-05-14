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
	// 초기값 설정
	Gait = E_Gait::Walk;
	Stance = E_Stance::Stand;
	MovementMode = E_MovementMode::OnGround;
	MovementState = E_MovementState::Idle;
}

void UGP_MotionMatchingAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	// 1. 공통 데이터 업데이트 (Velocity, GroundSpeed 등)
	Super::NativeUpdateAnimation(DeltaSeconds);
	StartElapsedTime += DeltaSeconds;

	// 2. 캐릭터 및 컴포넌트 유효성 확인 (부모 클래스에서 캐싱됨)
	if (!Character || !MovementComponent) return;

	// 3. Motion Matching 전용 상태 계산
	UpdateLocomotionStates();

	// 4. Chooser 기반 데이터베이스 선택 (확장 지원)
	UpdateMotionMatchingState();
}

void UGP_MotionMatchingAnimInstance::UpdateLocomotionStates()
{
	const E_Gait PrevGait = Gait;
	const E_Stance PrevStance = Stance;
	const E_MovementMode PrevMovementMode = MovementMode;
	const E_MovementState PrevMovementState = MovementState;
	const EMovementMode NativeMovementMode = MovementComponent->MovementMode;
	const bool bShouldBeInAir = bIsFalling || NativeMovementMode == MOVE_Falling;
	const bool bSprintTagActive = HasSprintTag(Character);
	const float MaxSpeed = FMath::Max(MovementComponent->GetMaxSpeed(), 1.0f);
	const float DynamicWalkToRunThreshold = FMath::Clamp(MaxSpeed * WalkToRunSpeedRatio, IdleSpeedThreshold + 1.0f, MaxSpeed);
	const bool bSprintSpeedAvailable = MaxSpeed >= MinimumSprintSpeed;
	const float DynamicRunToSprintThreshold = bSprintSpeedAvailable
		? FMath::Clamp(MaxSpeed * RunToSprintSpeedRatio, DynamicWalkToRunThreshold + 1.0f, MaxSpeed)
		: TNumericLimits<float>::Max();
	const float AccelerationMagnitude = Acceleration.Size2D();
	const bool bHasMeaningfulAcceleration = AccelerationMagnitude >= StartMinAccelerationThreshold;
	const bool bHasDedicatedRunBand = MaxSpeed > MinimumSprintSpeed;
	const bool bIsMovingNow = GroundSpeed >= StartMinSpeedThreshold;
	const bool bJustStartedMoving = !bWasMovingLastFrame && bIsMovingNow && bHasMeaningfulAcceleration;
	const bool bWithinStartWindow = StartElapsedTime <= StartWindowSeconds;

	// 3-1. Stance 계산
	Stance = bIsCrouching ? E_Stance::Crouch : E_Stance::Stand;

	// 3-2. MovementMode 계산
	MovementMode = bShouldBeInAir ? E_MovementMode::InAir : E_MovementMode::OnGround;

	// 3-3. Gait 계산 (현재 속도 및 스프린트 의도 기반)
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

	// 3-4. MovementState 계산
	if (MovementMode == E_MovementMode::InAir)
	{
		// 공중에 있을 때는 Moving 상태로 간주 (필요 시 Jump/Fall 분리 가능)
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
		&& GroundSpeed >= StartMinSpeedThreshold
		&& GroundSpeed <= StartMaxSpeedThreshold;

	IsPivoting = MovementMode == E_MovementMode::OnGround
		&& MovementState == E_MovementState::Moving
		&& GroundSpeed >= PivotMinSpeedThreshold
		&& bHasMeaningfulAcceleration
		&& FMath::Abs(LocalVelocityAngleDegrees) >= PivotMinAngleDegrees;

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
	// [알림] 현재 단계에서는 상태 계산까지만 C++에서 담당합니다.
	// 실제 SelectedPoseSearchDatabase의 결정은 AnimBP에서 Chooser Table을 평가하여
	// 본 함수를 Override하고 SetSelectedPoseSearchDatabase()를 호출하는 방식으로 확장합니다.
}
