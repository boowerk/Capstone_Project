#include "Animation/GP_MotionMatchingAnimInstance.h"
#include "PoseSearch/PoseSearchDatabase.h"

UGP_MotionMatchingAnimInstance::UGP_MotionMatchingAnimInstance()
{
	Gait = E_Gait::Walk;
	Stance = E_Stance::Stand;
	MovementMode = E_MovementMode::OnGround;
	MovementState = E_MovementState::Idle;
}

void UGP_MotionMatchingAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	UpdateMotionMatchingState();
}

void UGP_MotionMatchingAnimInstance::UpdateMotionMatchingState()
{
	// TODO: Chooser 평가 및 SelectedPoseSearchDatabase 갱신 로직
}
