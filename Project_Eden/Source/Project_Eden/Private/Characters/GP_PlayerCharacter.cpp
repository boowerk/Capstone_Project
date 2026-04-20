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
	PrimaryActorTick.bStartWithTickEnabled = false; // 이제 Tick은 사실상 돌지 않습니다.

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.2f;
	
	// 초기 속도 세팅
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed; 
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	
	// 물리 제동 마찰력 (추후 블루프린트에서 제어하여 슬라이딩 거리를 조절합니다)
	GetCharacterMovement()->BrakingDecelerationWalking = 1000.f; 

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>("CameraBoom");
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = 600.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>("FollowCamera");
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

void AGP_PlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	ApplyGroundMovementSpeed();
}

void AGP_PlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	// 롤 몽타주가 끝났는지 체크하는 간단한 용도로만 남김 (추후 GAS 이관 시 삭제)
	if (bIsRolling)
	{
		const UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
		UAnimMontage* RollMontage = GetRollMontage();
		if (!AnimInstance || !AnimInstance->Montage_IsPlaying(RollMontage))
		{
			bIsRolling = false;
			SetActorTickEnabled(false);
		}
	}
}

void AGP_PlayerCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	// 하드코딩된 Landed 몽타주 재생 로직 삭제 -> AnimBP의 State Machine에서 착지를 처리하도록 위임
}

// ==========================================
// GAS 및 초기화 로직 (기존 유지)
// ==========================================
UAbilitySystemComponent* AGP_PlayerCharacter::GetAbilitySystemComponent() const
{
	AGP_PlayerState* GPPlayerState = Cast<AGP_PlayerState>(GetPlayerState());
	return GPPlayerState ? GPPlayerState->GetAbilitySystemComponent() : nullptr;
}

UAttributeSet* AGP_PlayerCharacter::GetAttributeSet() const
{
	AGP_PlayerState* GPPlayerState = Cast<AGP_PlayerState>(GetPlayerState());
	return GPPlayerState ? GPPlayerState->GetAttributeSet() : nullptr;
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

// ==========================================
// 핵심 이동 로직 (완전 리팩토링)
// ==========================================
void AGP_PlayerCharacter::SetSprinting(bool bShouldSprint)
{
	if (bIsSprinting == bShouldSprint) return;
	
	bIsSprinting = bShouldSprint;
	ApplyGroundMovementSpeed();
}

void AGP_PlayerCharacter::ApplyGroundMovementSpeed()
{
	if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
	{
		// 입력 즉시 딜레이 없이 최고 속도 갱신
		MovementComp->MaxWalkSpeed = bIsSprinting ? SprintSpeed : WalkSpeed;
	}
}

// ==========================================
// 전투 및 롤 로직 (1차 유지, 2차 때 GAS로 이관)
// ==========================================
bool AGP_PlayerCharacter::TryPerformRoll()
{
	if (!GetWorld() || !GetMesh() || !GetCharacterMovement() || bIsRolling || GetCharacterMovement()->IsFalling()) return false;
	
	const double CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime < NextRollAllowedTime) return false;

	UAnimMontage* Montage = GetRollMontage();
	if (!IsValid(Montage)) return false;

	FVector RollDirection = GetLastMovementInputVector();
	if (RollDirection.IsNearlyZero()) RollDirection = GetVelocity();
	if (RollDirection.IsNearlyZero()) RollDirection = GetActorForwardVector();
	
	RollDirection.Z = 0.0f;
	RollDirection.Normalize();
	SetActorRotation(RollDirection.Rotation());

	if (PlayAnimMontage(Montage, 1.0f) > 0.0f)
	{
		GetCharacterMovement()->StopMovementImmediately();
		ConsumeMovementInputVector();
		bIsRolling = true;
		SetActorTickEnabled(true);
		NextRollAllowedTime = CurrentTime + RollCooldown;
		return true;
	}
	return false;
}

void AGP_PlayerCharacter::RequestPrimaryAttack(EGPPrimaryAttackType AttackType)
{
	RequestedPrimaryAttackType = AttackType;
	if (!bIsPrimaryAttacking) return;
	if (AttackType != ActivePrimaryAttackType) return;
	const int32 NextComboIndex = PrimaryAttackComboIndex + 1;
	if (IsValid(GetPrimaryAttackMontageForStep(AttackType, NextComboIndex)))
	{
		bHasQueuedPrimaryAttackCombo = true;
	}
}

UAnimMontage* AGP_PlayerCharacter::StartPrimaryAttackCombo()
{
	if (bIsPrimaryAttacking) return nullptr;
	bHasQueuedPrimaryAttackCombo = false;
	const EGPPrimaryAttackType PreviousAttackType = ActivePrimaryAttackType;
	const double CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	const bool bCanContinueCombo = PrimaryAttackComboIndex != INDEX_NONE && RequestedPrimaryAttackType == PreviousAttackType && CurrentTime <= PrimaryAttackComboExpireTime && IsValid(GetPrimaryAttackMontageForStep(PreviousAttackType, PrimaryAttackComboIndex + 1));
	ActivePrimaryAttackType = bCanContinueCombo ? PreviousAttackType : RequestedPrimaryAttackType;
	PrimaryAttackComboIndex = bCanContinueCombo ? PrimaryAttackComboIndex + 1 : 0;
	PrimaryAttackComboExpireTime = 0.0;
	UAnimMontage* ComboMontage = GetPrimaryAttackMontageForStep(ActivePrimaryAttackType, PrimaryAttackComboIndex);
	return IsValid(ComboMontage) ? ComboMontage : GetPrimaryAttackMontage();
}

UAnimMontage* AGP_PlayerCharacter::AdvancePrimaryAttackCombo()
{
	if (!bHasQueuedPrimaryAttackCombo) return nullptr;
	bHasQueuedPrimaryAttackCombo = false;
	const int32 NextComboIndex = PrimaryAttackComboIndex + 1;
	UAnimMontage* NextMontage = GetPrimaryAttackMontageForStep(ActivePrimaryAttackType, NextComboIndex);
	if (!IsValid(NextMontage)) return nullptr;
	PrimaryAttackComboIndex = NextComboIndex;
	PrimaryAttackComboExpireTime = 0.0;
	return NextMontage;
}

void AGP_PlayerCharacter::FinishPrimaryAttackCombo()
{
	SetPrimaryAttackActive(false);
	bHasQueuedPrimaryAttackCombo = false;
	const bool bHasNextComboStep = IsValid(GetPrimaryAttackMontageForStep(ActivePrimaryAttackType, PrimaryAttackComboIndex + 1));
	if (GetWorld() && bHasNextComboStep && PrimaryAttackComboGraceTime > 0.0f)
	{
		PrimaryAttackComboExpireTime = GetWorld()->GetTimeSeconds() + PrimaryAttackComboGraceTime;
	}
	else
	{
		PrimaryAttackComboIndex = INDEX_NONE;
		PrimaryAttackComboExpireTime = 0.0;
	}
}

void AGP_PlayerCharacter::CancelPrimaryAttackCombo()
{
	SetPrimaryAttackActive(false);
	bHasQueuedPrimaryAttackCombo = false;
	PrimaryAttackComboIndex = INDEX_NONE;
	PrimaryAttackComboExpireTime = 0.0;
}

void AGP_PlayerCharacter::SetPrimaryAttackActive(bool bIsActive)
{
	bIsPrimaryAttacking = bIsActive;
	if (!bIsPrimaryAttacking)
	{
		bHasQueuedPrimaryAttackCombo = false;
		return;
	}
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}
	ConsumeMovementInputVector();
}

// ==========================================
// Getter Helper 함수
// ==========================================
UBlendSpace* AGP_PlayerCharacter::GetLocomotionBlendSpace() const { return AnimationSet ? AnimationSet->LocomotionBlendSpace : nullptr; }
UAnimSequenceBase* AGP_PlayerCharacter::GetJumpLoopAnimation() const { return AnimationSet ? AnimationSet->JumpLoopAnimation : nullptr; }
UAnimMontage* AGP_PlayerCharacter::GetLandingMontage() const { return AnimationSet ? AnimationSet->LandingMontage : nullptr; }
UAnimMontage* AGP_PlayerCharacter::GetRollMontage() const { return AnimationSet ? AnimationSet->RollMontage : nullptr; }
UAnimMontage* AGP_PlayerCharacter::GetPrimaryAttackMontage() const
{
	if (!AnimationSet) return nullptr;
	if (AnimationSet->LightAttackMontages.IsValidIndex(0) && IsValid(AnimationSet->LightAttackMontages[0])) return AnimationSet->LightAttackMontages[0];
	return AnimationSet->PrimaryAttackMontage;
}
UAnimMontage* AGP_PlayerCharacter::GetPrimaryAttackMontageForStep(EGPPrimaryAttackType AttackType, int32 ComboIndex) const
{
	if (!AnimationSet || ComboIndex < 0) return nullptr;
	const TArray<TObjectPtr<UAnimMontage>>& ComboMontages = AttackType == EGPPrimaryAttackType::Heavy ? AnimationSet->HeavyAttackMontages : AnimationSet->LightAttackMontages;
	return ComboMontages.IsValidIndex(ComboIndex) ? ComboMontages[ComboIndex].Get() : nullptr;
}
