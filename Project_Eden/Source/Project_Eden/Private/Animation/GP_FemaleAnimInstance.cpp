#include "Animation/GP_FemaleAnimInstance.h"

#include "Animation/AnimSequenceBase.h"
#include "Animation/BlendSpace.h"
#include "Characters/GP_PlayerCharacter.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void UGP_FemaleAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	// 구르기 같은 액션 몽타주의 루트모션만 실제 캐릭터 이동으로 반영한다.
	RootMotionMode = ERootMotionMode::RootMotionFromMontagesOnly;

	CachedCharacter = Cast<ACharacter>(TryGetPawnOwner());
	CachedPlayerCharacter = Cast<AGP_PlayerCharacter>(CachedCharacter.Get());
	RefreshAnimationAssets();
}

void UGP_FemaleAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	ACharacter* Character = CachedCharacter.Get();
	if (!IsValid(Character))
	{
		Character = Cast<ACharacter>(TryGetPawnOwner());
		CachedCharacter = Character;
		CachedPlayerCharacter = Cast<AGP_PlayerCharacter>(Character);
		RefreshAnimationAssets();
	}

	if (!IsValid(Character))
	{
		LocomotionBlendSpaceAsset = nullptr;
		JumpLoopAnimationAsset = nullptr;
		GroundSpeed = 0.0f;
		bIsFalling = false;
		return;
	}

	if (!LocomotionBlendSpaceAsset || !JumpLoopAnimationAsset)
	{
		RefreshAnimationAssets();
	}

	GroundSpeed = Character->GetVelocity().Size2D();

	if (const UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement())
	{
		bIsFalling = MovementComponent->IsFalling();
	}
	else
	{
		bIsFalling = false;
	}
}

void UGP_FemaleAnimInstance::RefreshAnimationAssets()
{
	AGP_PlayerCharacter* PlayerCharacter = CachedPlayerCharacter.Get();
	if (!IsValid(PlayerCharacter))
	{
		PlayerCharacter = Cast<AGP_PlayerCharacter>(TryGetPawnOwner());
		CachedPlayerCharacter = PlayerCharacter;
	}

	if (!IsValid(PlayerCharacter))
	{
		LocomotionBlendSpaceAsset = nullptr;
		JumpLoopAnimationAsset = nullptr;
		return;
	}

	LocomotionBlendSpaceAsset = PlayerCharacter->GetLocomotionBlendSpace();
	JumpLoopAnimationAsset = PlayerCharacter->GetJumpLoopAnimation();
}
