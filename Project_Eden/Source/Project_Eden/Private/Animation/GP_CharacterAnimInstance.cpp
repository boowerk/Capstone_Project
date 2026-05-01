#include "Animation/GP_CharacterAnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void UGP_CharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Character = Cast<ACharacter>(TryGetPawnOwner());
	if (Character)
	{
		MovementComponent = Character->GetCharacterMovement();
	}
}

void UGP_CharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!Character || !MovementComponent)
	{
		// 유효하지 않을 경우 재시도
		Character = Cast<ACharacter>(TryGetPawnOwner());
		if (Character)
		{
			MovementComponent = Character->GetCharacterMovement();
		}
	}

	// 데이터 에셋으로부터 애니메이션 캐싱 (동적 교체 지원)
	if (AnimationSet)
	{
		LocomotionBlendSpace = AnimationSet->LocomotionBlendSpace;
		JumpLoopAnimation = AnimationSet->JumpLoopAnimation;
		SprintStopAnimation = AnimationSet->SprintStopAnimation;
	}

	if (!Character || !MovementComponent) return;

	// 속도 및 이동 데이터 추출
	FVector WorldVelocity = Character->GetVelocity();
	GroundSpeed = WorldVelocity.Size2D();
	
	// 캐릭터 로컬 방향으로 변환
	LocalVelocityDirection = Character->GetActorTransform().InverseTransformVectorNoScale(WorldVelocity).GetSafeNormal2D();

	// 상태 데이터 업데이트
	bHasAcceleration = MovementComponent->GetCurrentAcceleration().SizeSquared2D() > KINDA_SMALL_NUMBER;
	bIsFalling = MovementComponent->IsFalling();
	bIsAnyMontagePlaying = Montage_IsPlaying(nullptr);
}
