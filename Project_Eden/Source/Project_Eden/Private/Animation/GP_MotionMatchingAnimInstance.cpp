#include "Animation/GP_MotionMatchingAnimInstance.h"
#include "PoseSearch/PoseSearchDatabase.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

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

	// 2. 캐릭터 및 컴포넌트 유효성 재확인 (부모 클래스에서 캐싱됨)
	if (!Character || !MovementComponent) return;

	// 3. Motion Matching 전용 상태 계산
	UpdateLocomotionStates();

	// 4. Chooser 기반 데이터베이스 선택 (확장 지점)
	UpdateMotionMatchingState();
}

void UGP_MotionMatchingAnimInstance::UpdateLocomotionStates()
{
	// 3-1. Stance 계산
	Stance = bIsCrouching ? E_Stance::Crouch : E_Stance::Stand;

	// 3-2. MovementMode 계산
	MovementMode = bIsFalling ? E_MovementMode::InAir : E_MovementMode::OnGround;

	// 3-3. Gait 계산 (GroundSpeed 기반)
	if (GroundSpeed < WalkToRunThreshold)
	{
		Gait = E_Gait::Walk;
	}
	else if (GroundSpeed < RunToSprintThreshold)
	{
		Gait = E_Gait::Run;
	}
	else
	{
		Gait = E_Gait::Sprint;
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
}

void UGP_MotionMatchingAnimInstance::UpdateMotionMatchingState()
{
	// [알림] 현재 단계에서는 상태 계산(Input 축 준비)까지만 C++에서 담당합니다.
	// 실제 SelectedPoseSearchDatabase의 결정은 블루프린트에서 Chooser Table을 평가하여 수행하거나,
	// 향후 Chooser API를 직접 호출하는 로직으로 확장될 예정입니다.
	
	// TODO: SelectedPoseSearchDatabase = Chooser->Evaluate(...);
}
