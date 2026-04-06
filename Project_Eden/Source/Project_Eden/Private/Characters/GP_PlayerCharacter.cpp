#include "Characters/GP_PlayerCharacter.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/BlendSpace.h"
#include "Animation/PDA_CharacterAnimationSet.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Player/GP_PlayerState.h"
#include "AbilitySystemComponent.h"

AGP_PlayerCharacter::AGP_PlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.2f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 1500.f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>("CameraBoom");
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = 600.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>("FollowCamera");
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

void AGP_PlayerCharacter::Landed(const FHitResult& Hit)
{
	const float LandingSpeed = -GetVelocity().Z;

	Super::Landed(Hit);

	if (LandingSpeed < MinLandingSpeedForMontage)
	{
		return;
	}

	UAnimMontage* LandingMontage = GetLandingMontage();
	if (!IsValid(LandingMontage) || !GetMesh())
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		if (!AnimInstance->Montage_IsPlaying(LandingMontage))
		{
			if (PlayAnimMontage(LandingMontage, 1.0f) > 0.0f)
			{
				ActiveLandingElapsedTime = 0.0f;
				ActiveLandingMontage = LandingMontage;
				SetActorTickEnabled(true);
			}
		}
	}
}

void AGP_PlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateLandingAnimation(DeltaSeconds);

	if (bIsRolling)
	{
		UpdateRollMovement(DeltaSeconds);
	}

	if (!bIsRolling && !ActiveLandingMontage.IsValid())
	{
		SetActorTickEnabled(false);
	}
}

UAbilitySystemComponent* AGP_PlayerCharacter::GetAbilitySystemComponent() const
{
	AGP_PlayerState* GPPlayerState = Cast<AGP_PlayerState>(GetPlayerState());
	if (!IsValid(GPPlayerState))return nullptr;

	return GPPlayerState->GetAbilitySystemComponent();
}

UAttributeSet* AGP_PlayerCharacter::GetAttributeSet() const
{
	AGP_PlayerState* GPPlayerState = Cast<AGP_PlayerState>(GetPlayerState());
	if (!IsValid(GPPlayerState)) return nullptr;

	return GPPlayerState->GetAttributeSet();
}

void AGP_PlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (!IsValid(GetAbilitySystemComponent()) || !HasAuthority()) return;

	GetAbilitySystemComponent()->InitAbilityActorInfo(GetPlayerState(), this);
	OnASCInitialized.Broadcast(GetAbilitySystemComponent(), GetAttributeSet());
	GiveStartupAbilities();
	InitializeAttributes();
}

void AGP_PlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	if (!IsValid(GetAbilitySystemComponent())) return;

	GetAbilitySystemComponent()->InitAbilityActorInfo(GetPlayerState(), this);
	OnASCInitialized.Broadcast(GetAbilitySystemComponent(), GetAttributeSet());
}

void AGP_PlayerCharacter::SetPrimaryAttackActive(bool bIsActive)
{
	bIsPrimaryAttacking = bIsActive;

	if (!bIsPrimaryAttacking)
	{
		return;
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	ConsumeMovementInputVector();
}

UBlendSpace* AGP_PlayerCharacter::GetLocomotionBlendSpace() const
{
	return AnimationSet ? AnimationSet->LocomotionBlendSpace : nullptr;
}

UAnimSequenceBase* AGP_PlayerCharacter::GetJumpLoopAnimation() const
{
	return AnimationSet ? AnimationSet->JumpLoopAnimation : nullptr;
}

UAnimMontage* AGP_PlayerCharacter::GetLandingMontage() const
{
	return AnimationSet ? AnimationSet->LandingMontage : nullptr;
}

UAnimMontage* AGP_PlayerCharacter::GetRollMontage() const
{
	return AnimationSet ? AnimationSet->RollMontage : nullptr;
}

UAnimMontage* AGP_PlayerCharacter::GetPrimaryAttackMontage() const
{
	return AnimationSet ? AnimationSet->PrimaryAttackMontage : nullptr;
}

void AGP_PlayerCharacter::UpdateLandingAnimation(float DeltaSeconds)
{
	if (!ActiveLandingMontage.IsValid() || !GetMesh())
	{
		ActiveLandingElapsedTime = 0.0f;
		ActiveLandingMontage.Reset();
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance || !AnimInstance->Montage_IsPlaying(ActiveLandingMontage.Get()))
	{
		ActiveLandingElapsedTime = 0.0f;
		ActiveLandingMontage.Reset();
		return;
	}

	ActiveLandingElapsedTime += DeltaSeconds;

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		if (!MovementComponent->IsFalling() && ActiveLandingElapsedTime >= MinLandingPlayTimeBeforeBlendOut)
		{
			const bool bHasMoveInput = MovementComponent->GetCurrentAcceleration().SizeSquared2D() > KINDA_SMALL_NUMBER;
			if (bHasMoveInput)
			{
				AnimInstance->Montage_Stop(LandingMontageBlendOutTime, ActiveLandingMontage.Get());
				ActiveLandingElapsedTime = 0.0f;
				ActiveLandingMontage.Reset();
			}
		}
	}
}

bool AGP_PlayerCharacter::TryPerformRoll()
{
	if (!GetWorld() || !GetMesh() || !GetCharacterMovement())
	{
		return false;
	}

	if (bIsRolling)
	{
		return false;
	}

	if (GetCharacterMovement()->IsFalling())
	{
		return false;
	}

	const double CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime < NextRollAllowedTime)
	{
		return false;
	}

	UAnimMontage* Montage = GetRollMontage();
	if (!IsValid(Montage))
	{
		return false;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		if (AnimInstance->Montage_IsPlaying(Montage))
		{
			return false;
		}
	}

	FVector RollDirection = GetLastMovementInputVector();
	if (RollDirection.SizeSquared2D() < KINDA_SMALL_NUMBER)
	{
		RollDirection = GetVelocity();
	}
	if (RollDirection.SizeSquared2D() < KINDA_SMALL_NUMBER)
	{
		RollDirection = GetActorForwardVector();
	}

	RollDirection.Z = 0.0f;
	RollDirection.Normalize();
	SetActorRotation(RollDirection.Rotation());

	const float PlayedLength = PlayAnimMontage(Montage, 1.0f);
	if (PlayedLength <= 0.0f)
	{
		return false;
	}

	GetCharacterMovement()->StopMovementImmediately();
	ConsumeMovementInputVector();

	bIsRolling = true;
	ActiveRollDirection = RollDirection;
	ActiveRollDuration = PlayedLength;
	ActiveRollElapsedTime = 0.0f;
	ActiveRollDistanceTravelled = 0.0f;
	ActiveRollMontage = Montage;
	SetActorTickEnabled(true);

	NextRollAllowedTime = CurrentTime + RollCooldown;
	return true;
}

void AGP_PlayerCharacter::UpdateRollMovement(float DeltaSeconds)
{
	if (!bIsRolling)
	{
		return;
	}

	if (!GetCharacterMovement())
	{
		FinishRoll();
		return;
	}

	ActiveRollElapsedTime = FMath::Min(ActiveRollElapsedTime + DeltaSeconds, ActiveRollDuration);

	const float Alpha = ActiveRollDuration > 0.0f ? ActiveRollElapsedTime / ActiveRollDuration : 1.0f;
	const float TargetDistance = RollDistance * Alpha;
	const float DeltaDistance = TargetDistance - ActiveRollDistanceTravelled;

	if (DeltaDistance > KINDA_SMALL_NUMBER)
	{
		const FVector PreviousLocation = GetActorLocation();
		FHitResult HitResult;
		AddActorWorldOffset(ActiveRollDirection * DeltaDistance, true, &HitResult);
		ActiveRollDistanceTravelled += FVector::Dist2D(PreviousLocation, GetActorLocation());

		if (HitResult.bBlockingHit)
		{
			ActiveRollDistanceTravelled = RollDistance;
		}
	}

	const UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	const bool bMontageStopped = !ActiveRollMontage.IsValid() || !AnimInstance || !AnimInstance->Montage_IsPlaying(ActiveRollMontage.Get());
	if (ActiveRollElapsedTime >= ActiveRollDuration || bMontageStopped)
	{
		FinishRoll();
	}
}

void AGP_PlayerCharacter::FinishRoll()
{
	bIsRolling = false;
	ActiveRollDirection = FVector::ZeroVector;
	ActiveRollDuration = 0.0f;
	ActiveRollElapsedTime = 0.0f;
	ActiveRollDistanceTravelled = 0.0f;
	ActiveRollMontage.Reset();

	SetActorTickEnabled(false);
}

