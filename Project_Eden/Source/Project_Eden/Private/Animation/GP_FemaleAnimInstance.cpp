#include "Animation/GP_FemaleAnimInstance.h"
#include "Characters/GP_PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

void UGP_FemaleAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	// 부모 클래스에서 Character와 MovementComponent를 이미 캐싱함
}

void UGP_FemaleAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!Character || !MovementComponent) return;

	// Female 전용 추가 로직 (예: 특정 가속도 임계값에 따른 Sprint Stop 판단)
	// 현재는 기존 변수 구조를 유지하기 위해 예시로 남겨둠
	bShouldSprintStop = (GroundSpeed < 10.f) && bHasAcceleration; 
}
