#include "Animation/GP_MotionMatchingAnimInstance.h"
#include "PoseSearch/PoseSearchDatabase.h"

UGP_MotionMatchingAnimInstance::UGP_MotionMatchingAnimInstance()
{
	// 승격된 C++ 에넘 타입으로 초기화
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
	// TODO: 마이그레이션 단계 5에서 Chooser 평가 결과와 연동할 예정입니다.
}
