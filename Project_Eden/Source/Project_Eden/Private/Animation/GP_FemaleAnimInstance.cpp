#include "Animation/GP_FemaleAnimInstance.h"

#include "Characters/GP_PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

void UGP_FemaleAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Character = Cast<AGP_PlayerCharacter>(TryGetPawnOwner());
	if (Character)
	{
		MovementComponent = Character->GetCharacterMovement();
	}
}

void UGP_FemaleAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!Character || !MovementComponent)
	{
		return;
	}

	// 1. 기본 이동 데이터 갱신
	GroundSpeed = Character->GetVelocity().Size2D();
	bHasAcceleration = MovementComponent->GetCurrentAcceleration().SizeSquared2D() > KINDA_SMALL_NUMBER;
	bIsFalling = MovementComponent->IsFalling();

	// 2. 미끄러짐(Sprint Stop) 진입 조건 계산 (가장 중요)
	// 달리기 속도(300 이상)로 이동하다가 손가락을 뗐을 때(가속도 0) 발동
	bShouldSprintStop = (GroundSpeed > 300.f) && (!bHasAcceleration) && (!bIsFalling);
}
