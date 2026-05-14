#include "Animation/GP_MotionMatchingAnimInstance.h"
#include "PoseSearch/PoseSearchDatabase.h"

UGP_MotionMatchingAnimInstance::UGP_MotionMatchingAnimInstance()
{
	// 초기값은 기존 에넘의 기본 인덱스(0)를 사용
	Gait = 0;
	Stance = 0;
	MovementMode = 0;
	MovementState = 0;
}

void UGP_MotionMatchingAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	UpdateMotionMatchingState();
}

void UGP_MotionMatchingAnimInstance::UpdateMotionMatchingState()
{
	// Chooser 평가 및 SelectedPoseSearchDatabase 갱신 로직이 들어갈 자리
}
