#include "Animation/GP_RetargetAnimInstance.h"

UGP_RetargetAnimInstance::UGP_RetargetAnimInstance()
{
	Gait = E_Gait::Walk;
	Stance = E_Stance::Stand;
	ExperimentalState = E_ExperimentalStateMachineState::Idle_Loop;
	MovementDirection = E_MovementDirection::Forward;
	GroundSpeed = 0.0f;
}

void UGP_RetargetAnimInstance::SyncLocomotionState(E_Gait InGait, E_Stance InStance, E_ExperimentalStateMachineState InExpState, E_MovementDirection InDirection, float InSpeed)
{
	Gait = InGait;
	Stance = InStance;
	ExperimentalState = InExpState;
	MovementDirection = InDirection;
	GroundSpeed = InSpeed;
}
