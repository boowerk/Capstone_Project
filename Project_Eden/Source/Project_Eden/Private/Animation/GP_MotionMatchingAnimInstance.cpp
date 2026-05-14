#include "Animation/GP_MotionMatchingAnimInstance.h"
#include "PoseSearch/PoseSearchDatabase.h"

UGP_MotionMatchingAnimInstance::UGP_MotionMatchingAnimInstance()
{
	Gait = E_Gait::Walking;
	Stance = E_Stance::Standing;
	MovementMode = E_MovementMode::Grounded;
	MovementState = E_MovementState::Idle;
}

void UGP_MotionMatchingAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	UpdateMotionMatchingState();
}

void UGP_MotionMatchingAnimInstance::UpdateMotionMatchingState()
{
	// TODO: Chooser 평가 로직 구현 시 SelectedPoseSearchDatabase를 갱신합니다.
}
