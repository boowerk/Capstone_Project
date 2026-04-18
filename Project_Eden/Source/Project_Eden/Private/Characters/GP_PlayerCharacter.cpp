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

namespace GPSprintLocomotion
{
	const FName LocomotionSyncGroupName(TEXT("Locomotion"));
	const FName LeftPlantMarkerName(TEXT("LeftPlant"));
	const FName RightPlantMarkerName(TEXT("RightPlant"));
}

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

void AGP_PlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 블루프린트에서 속도 값을 바꿔도 시작 시점에는 걷기 속도를 다시 적용한다.
	ApplyGroundMovementSpeed();
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
	UpdatePendingSprintEnter(DeltaSeconds);
	UpdateSprintSpeedTransition(DeltaSeconds);

	if (bIsRolling)
	{
		UpdateRollState();
	}

	if (!bIsRolling && !bIsSprintEnterPending && !bIsSprintSpeedTransitionActive && !ActiveLandingMontage.IsValid())
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

void AGP_PlayerCharacter::SetSprinting(bool bShouldSprint)
{
	if (bShouldSprint && bIsSprintExitControlLocked)
	{
		// Exit 슬라이드 잠금 중에는 방향 전환이나 재스프린트를 허용하지 않는다.
		return;
	}

	if (bIsSprinting == bShouldSprint)
	{
		return;
	}

	bIsSprinting = bShouldSprint;
	if (!bIsSprinting)
	{
		ClearPendingSprintEnter();
	}

	StartSprintSpeedTransition(bShouldSprint);
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

UAnimMontage* AGP_PlayerCharacter::GetSprintEnterLeftMontage() const
{
	return AnimationSet ? AnimationSet->SprintEnterLeftMontage : nullptr;
}

UAnimMontage* AGP_PlayerCharacter::GetSprintEnterRightMontage() const
{
	return AnimationSet ? AnimationSet->SprintEnterRightMontage : nullptr;
}

UAnimMontage* AGP_PlayerCharacter::GetSprintExitLeftMontage() const
{
	return AnimationSet ? AnimationSet->SprintExitLeftMontage : nullptr;
}

UAnimMontage* AGP_PlayerCharacter::GetSprintExitRightMontage() const
{
	return AnimationSet ? AnimationSet->SprintExitRightMontage : nullptr;
}

void AGP_PlayerCharacter::ApplyGroundMovementSpeed()
{
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		// Shift를 누를 때만 달리고, 평소에는 걷기 속도를 유지한다.
		MovementComponent->MaxWalkSpeed = bIsSprinting ? SprintSpeed : WalkSpeed;
	}
}

void AGP_PlayerCharacter::StartSprintSpeedTransition(bool bShouldSprint, bool bBypassSprintEnterMarkerGate)
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		return;
	}

	if (bShouldSprint && bIsSprintExitControlLocked)
	{
		return;
	}

	StopActiveSprintTransitionMontage();

	if (bShouldSprint && !bBypassSprintEnterMarkerGate && ShouldDelaySprintEnterForMarker())
	{
		// 걷기/조깅 중에는 LeftPlant/RightPlant 근처에서만 Enter를 시작해 발 위상이 겹치지 않게 한다.
		bIsSprintEnterPending = true;
		SprintEnterPendingElapsedTime = 0.0f;
		bIsSprintSpeedTransitionActive = false;
		ActiveSprintTransitionMontage.Reset();
		LogSprintMarkerPhase(SelectSprintEnterMontage());
		SetActorTickEnabled(true);
		return;
	}

	ClearPendingSprintEnter();

	const bool bShouldPlayExitMontage = !bShouldSprint && HasReachedSprintExitSpeed();
	UAnimMontage* TransitionMontage = bShouldSprint ? SelectSprintEnterMontage() : (bShouldPlayExitMontage ? SelectSprintExitMontage() : nullptr);
	const bool bCanPlaySprintMontage = IsValid(TransitionMontage) && GetMesh() && !bIsRolling && !bIsPrimaryAttacking && !MovementComponent->IsFalling();

	float PlayedLength = 0.0f;
	if (bCanPlaySprintMontage)
	{
		PlayedLength = PlayAnimMontage(TransitionMontage, 1.0f);
		LogSprintMarkerPhase(TransitionMontage);
	}

	if (PlayedLength <= 0.0f)
	{
		bIsSprintSpeedTransitionActive = false;
		ActiveSprintTransitionMontage.Reset();
		ApplyGroundMovementSpeed();
		return;
	}

	// Enter/Exit 애니메이션 길이에 맞춰 MaxWalkSpeed를 서서히 바꾸면 시각 블렌드와 실제 이동이 같이 맞는다.
	SprintSpeedTransitionStart = MovementComponent->MaxWalkSpeed;
	SprintSpeedTransitionTarget = bShouldSprint ? SprintSpeed : WalkSpeed;
	SprintSpeedTransitionElapsedTime = 0.0f;
	SprintSpeedTransitionDuration = PlayedLength;
	SprintSpeedRampDuration = bShouldSprint ? FMath::Max(PlayedLength * SprintEnterSpeedRampRatio, KINDA_SMALL_NUMBER) : PlayedLength;
	ActiveSprintTransitionMontage = TransitionMontage;
	bIsSprintExitTransitionActive = !bShouldSprint && bShouldPlayExitMontage;
	bIsSprintExitControlLocked = bIsSprintExitTransitionActive;
	SprintExitSlideDirection = !GetVelocity().IsNearlyZero() ? GetVelocity().GetSafeNormal2D() : GetActorForwardVector().GetSafeNormal2D();
	bIsSprintSpeedTransitionActive = true;
	SetActorTickEnabled(true);
}

void AGP_PlayerCharacter::UpdatePendingSprintEnter(float DeltaSeconds)
{
	if (!bIsSprintEnterPending)
	{
		return;
	}

	SprintEnterPendingElapsedTime += DeltaSeconds;
	if (!bIsSprinting)
	{
		ClearPendingSprintEnter();
		return;
	}

	const bool bCanStartOnMarker = IsSprintEnterMarkerAligned();
	const bool bWaitExpired = SprintEnterPendingElapsedTime >= SprintEnterMaxMarkerWaitTime;
	if (!bCanStartOnMarker && !bWaitExpired)
	{
		return;
	}

	// 입력 반응성이 너무 늦어지지 않도록, 마커를 못 잡아도 짧은 대기 뒤에는 Enter를 시작한다.
	ClearPendingSprintEnter();
	StartSprintSpeedTransition(true, true);
}

void AGP_PlayerCharacter::UpdateSprintSpeedTransition(float DeltaSeconds)
{
	if (!bIsSprintSpeedTransitionActive)
	{
		return;
	}

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		FinishSprintSpeedTransition();
		return;
	}

	bool bMontageStopped = false;
	if (ActiveSprintTransitionMontage.IsValid() && GetMesh())
	{
		const UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		bMontageStopped = !AnimInstance || !AnimInstance->Montage_IsPlaying(ActiveSprintTransitionMontage.Get());
	}

	SprintSpeedTransitionElapsedTime = FMath::Min(SprintSpeedTransitionElapsedTime + DeltaSeconds, SprintSpeedTransitionDuration);
	const float SpeedAlpha = SprintSpeedRampDuration > 0.0f ? FMath::Min(SprintSpeedTransitionElapsedTime / SprintSpeedRampDuration, 1.0f) : 1.0f;
	const float SpeedBlendAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, SpeedAlpha, 2.0f);
	MovementComponent->MaxWalkSpeed = FMath::Lerp(SprintSpeedTransitionStart, SprintSpeedTransitionTarget, SpeedBlendAlpha);

	const float MontageAlpha = SprintSpeedTransitionDuration > 0.0f ? SprintSpeedTransitionElapsedTime / SprintSpeedTransitionDuration : 1.0f;
	if (bIsSprintExitTransitionActive)
	{
		const float LockEndAlpha = FMath::Clamp(SprintExitControlLockRatio, 0.0f, 1.0f);
		bIsSprintExitControlLocked = MontageAlpha < LockEndAlpha;

		if (bIsSprintExitControlLocked && SprintExitSlideDistance > 0.0f && !SprintExitSlideDirection.IsNearlyZero() && SprintSpeedTransitionDuration > 0.0f)
		{
			// Exit 애니메이션 초반에는 입력 대신 기존 진행 방향으로 살짝 미끄러지게 만든다.
			const float LockDuration = FMath::Max(SprintSpeedTransitionDuration * LockEndAlpha, KINDA_SMALL_NUMBER);
			const float SlideDeltaScale = DeltaSeconds / LockDuration;
			AddActorWorldOffset(SprintExitSlideDirection * SprintExitSlideDistance * SlideDeltaScale, true);
		}
	}

	if (MontageAlpha >= 1.0f || bMontageStopped)
	{
		FinishSprintSpeedTransition();
	}
}

void AGP_PlayerCharacter::FinishSprintSpeedTransition()
{
	bIsSprintSpeedTransitionActive = false;
	bIsSprintExitTransitionActive = false;
	bIsSprintExitControlLocked = false;
	SprintSpeedTransitionElapsedTime = 0.0f;
	SprintSpeedTransitionDuration = 0.0f;
	SprintSpeedRampDuration = 0.0f;
	SprintExitSlideDirection = FVector::ZeroVector;
	ActiveSprintTransitionMontage.Reset();
	ApplyGroundMovementSpeed();

	if (!bIsRolling && !bIsSprintEnterPending && !ActiveLandingMontage.IsValid())
	{
		SetActorTickEnabled(false);
	}
}

void AGP_PlayerCharacter::StopActiveSprintTransitionMontage()
{
	if (!ActiveSprintTransitionMontage.IsValid() || !GetMesh())
	{
		ActiveSprintTransitionMontage.Reset();
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		if (AnimInstance->Montage_IsPlaying(ActiveSprintTransitionMontage.Get()))
		{
			AnimInstance->Montage_Stop(SprintTransitionInterruptBlendTime, ActiveSprintTransitionMontage.Get());
		}
	}

	ActiveSprintTransitionMontage.Reset();
	bIsSprintExitTransitionActive = false;
	bIsSprintExitControlLocked = false;
	SprintExitSlideDirection = FVector::ZeroVector;
}

bool AGP_PlayerCharacter::HasReachedSprintExitSpeed() const
{
	const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent || SprintSpeed <= 0.0f)
	{
		return false;
	}

	// 실제 최고속도에 거의 도달한 뒤에만 Exit 연출을 재생한다.
	const float RequiredExitSpeed = SprintSpeed * SprintExitMinSpeedRatio;
	return GetVelocity().Size2D() >= RequiredExitSpeed && MovementComponent->MaxWalkSpeed >= RequiredExitSpeed;
}

bool AGP_PlayerCharacter::ShouldDelaySprintEnterForMarker() const
{
	if (SprintEnterMarkerWindow <= 0.0f || SprintEnterMaxMarkerWaitTime <= 0.0f || IsIdleSprintStart())
	{
		return false;
	}

	const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent || MovementComponent->IsFalling() || bIsRolling || bIsPrimaryAttacking || !IsValid(SelectSprintEnterMontage()))
	{
		return false;
	}

	return !IsSprintEnterMarkerAligned();
}

bool AGP_PlayerCharacter::IsSprintEnterMarkerAligned() const
{
	if (!GetMesh())
	{
		return true;
	}

	const UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance)
	{
		return true;
	}

	float TimeToLeftPlant = 0.0f;
	float TimeToRightPlant = 0.0f;
	const bool bHasLeftPlantTime = AnimInstance->GetTimeToClosestMarker(GPSprintLocomotion::LocomotionSyncGroupName, GPSprintLocomotion::LeftPlantMarkerName, TimeToLeftPlant);
	const bool bHasRightPlantTime = AnimInstance->GetTimeToClosestMarker(GPSprintLocomotion::LocomotionSyncGroupName, GPSprintLocomotion::RightPlantMarkerName, TimeToRightPlant);
	if (!bHasLeftPlantTime && !bHasRightPlantTime)
	{
		return true;
	}

	const bool bNearLeftPlant = bHasLeftPlantTime && FMath::Abs(TimeToLeftPlant) <= SprintEnterMarkerWindow;
	const bool bNearRightPlant = bHasRightPlantTime && FMath::Abs(TimeToRightPlant) <= SprintEnterMarkerWindow;
	return bNearLeftPlant || bNearRightPlant;
}

bool AGP_PlayerCharacter::IsIdleSprintStart() const
{
	const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	const bool bHasMoveInput = MovementComponent && MovementComponent->GetCurrentAcceleration().SizeSquared2D() > KINDA_SMALL_NUMBER;
	return !bHasMoveInput && GetVelocity().SizeSquared2D() <= FMath::Square(WalkSpeed * 0.25f);
}

UAnimMontage* AGP_PlayerCharacter::SelectSprintEnterMontage() const
{
	if (IsIdleSprintStart())
	{
		// 정지 상태에는 발 위상이 없으므로 오른발 시작 Enter를 기본값으로 사용한다.
		return IsValid(GetSprintEnterRightMontage()) ? GetSprintEnterRightMontage() : GetSprintEnterLeftMontage();
	}

	return SelectSprintTransitionMontageByPlant(GetSprintEnterLeftMontage(), GetSprintEnterRightMontage());
}

UAnimMontage* AGP_PlayerCharacter::SelectSprintExitMontage() const
{
	return SelectSprintTransitionMontageByPlant(GetSprintExitLeftMontage(), GetSprintExitRightMontage());
}

UAnimMontage* AGP_PlayerCharacter::SelectSprintTransitionMontageByPlant(UAnimMontage* LeftMontage, UAnimMontage* RightMontage) const
{
	if (!IsValid(LeftMontage) && !IsValid(RightMontage))
	{
		return nullptr;
	}

	auto SelectLeft = [LeftMontage, RightMontage]()
	{
		return IsValid(LeftMontage) ? LeftMontage : RightMontage;
	};

	auto SelectRight = [LeftMontage, RightMontage]()
	{
		return IsValid(RightMontage) ? RightMontage : LeftMontage;
	};

	if (!GetMesh())
	{
		return SelectRight();
	}

	const UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance)
	{
		return SelectRight();
	}

	float TimeToLeftPlant = 0.0f;
	float TimeToRightPlant = 0.0f;
	const bool bHasLeftPlantTime = AnimInstance->GetTimeToClosestMarker(GPSprintLocomotion::LocomotionSyncGroupName, GPSprintLocomotion::LeftPlantMarkerName, TimeToLeftPlant);
	const bool bHasRightPlantTime = AnimInstance->GetTimeToClosestMarker(GPSprintLocomotion::LocomotionSyncGroupName, GPSprintLocomotion::RightPlantMarkerName, TimeToRightPlant);
	if (bHasLeftPlantTime && bHasRightPlantTime)
	{
		// 현재 보행 위상에서 더 가까운 Plant 발과 같은 쪽 전환 애니메이션을 선택한다.
		return FMath::Abs(TimeToLeftPlant) <= FMath::Abs(TimeToRightPlant) ? SelectLeft() : SelectRight();
	}

	if (bHasLeftPlantTime)
	{
		return SelectLeft();
	}

	if (bHasRightPlantTime)
	{
		return SelectRight();
	}

	const bool bLeftToRightPhase = AnimInstance->IsSyncGroupBetweenMarkers(GPSprintLocomotion::LocomotionSyncGroupName, GPSprintLocomotion::LeftPlantMarkerName, GPSprintLocomotion::RightPlantMarkerName, true);
	const bool bRightToLeftPhase = AnimInstance->IsSyncGroupBetweenMarkers(GPSprintLocomotion::LocomotionSyncGroupName, GPSprintLocomotion::RightPlantMarkerName, GPSprintLocomotion::LeftPlantMarkerName, true);
	if (bLeftToRightPhase)
	{
		return SelectRight();
	}

	if (bRightToLeftPhase)
	{
		return SelectLeft();
	}

	return SelectRight();
}

void AGP_PlayerCharacter::ClearPendingSprintEnter()
{
	bIsSprintEnterPending = false;
	SprintEnterPendingElapsedTime = 0.0f;
}

void AGP_PlayerCharacter::LogSprintMarkerPhase(UAnimMontage* SelectedMontage) const
{
	if (!bDebugSprintMarkerPhase || !GetMesh())
	{
		return;
	}

	const UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	float TimeToLeftPlant = 0.0f;
	float TimeToRightPlant = 0.0f;
	const bool bHasLeftPlantTime = AnimInstance->GetTimeToClosestMarker(GPSprintLocomotion::LocomotionSyncGroupName, GPSprintLocomotion::LeftPlantMarkerName, TimeToLeftPlant);
	const bool bHasRightPlantTime = AnimInstance->GetTimeToClosestMarker(GPSprintLocomotion::LocomotionSyncGroupName, GPSprintLocomotion::RightPlantMarkerName, TimeToRightPlant);

	// 에디터에서 Notify_LeftFootDown/Notify_RightFootDown과 실제 전환 시점을 맞출 때 이 로그를 기준으로 본다.
	UE_LOG(LogTemp, Display, TEXT("[SprintMarker] Group=%s TimeToLeftPlant=%s TimeToRightPlant=%s Selected=%s"),
		*GPSprintLocomotion::LocomotionSyncGroupName.ToString(),
		bHasLeftPlantTime ? *FString::Printf(TEXT("%.3f"), TimeToLeftPlant) : TEXT("N/A"),
		bHasRightPlantTime ? *FString::Printf(TEXT("%.3f"), TimeToRightPlant) : TEXT("N/A"),
		*GetNameSafe(SelectedMontage));
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

	if (PlayAnimMontage(Montage, 1.0f) <= 0.0f)
	{
		return false;
	}

	GetCharacterMovement()->StopMovementImmediately();
	ConsumeMovementInputVector();

	// 루트모션 롤이 시작되기 전에 진행 방향만 맞춰준다.
	bIsRolling = true;
	ActiveRollMontage = Montage;
	SetActorTickEnabled(true);

	NextRollAllowedTime = CurrentTime + RollCooldown;
	return true;
}

void AGP_PlayerCharacter::UpdateRollState()
{
	if (!bIsRolling)
	{
		return;
	}

	if (!GetMesh())
	{
		FinishRoll();
		return;
	}

	const UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	const bool bMontageStopped = !ActiveRollMontage.IsValid() || !AnimInstance || !AnimInstance->Montage_IsPlaying(ActiveRollMontage.Get());
	if (bMontageStopped)
	{
		FinishRoll();
	}
}

void AGP_PlayerCharacter::FinishRoll()
{
	bIsRolling = false;
	ActiveRollMontage.Reset();

	SetActorTickEnabled(false);
}

