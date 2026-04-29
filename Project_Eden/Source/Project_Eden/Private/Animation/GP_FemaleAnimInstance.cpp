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

	if (!Character || !MovementComponent) return;

	FVector WorldVelocity = Character->GetVelocity();
	GroundSpeed = WorldVelocity.Size2D();
	LocalVelocityDirection = Character->GetActorTransform().InverseTransformVectorNoScale(WorldVelocity).GetSafeNormal2D();

	bHasAcceleration = MovementComponent->GetCurrentAcceleration().SizeSquared2D() > KINDA_SMALL_NUMBER;
	bIsFalling = MovementComponent->IsFalling();
	bIsAnyMontagePlaying = Montage_IsPlaying(nullptr);
}
