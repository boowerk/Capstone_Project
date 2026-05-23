#include "Characters/GP_PlayerCharacter.h"
#include "Animation/AnimInstance.h"
#include "Animation/GP_CharacterAnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/BlendSpace.h"
#include "Animation/AnimTypes.h"
#include "Animation/PDA_CharacterAnimationSet.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Player/GP_PlayerState.h"
#include "AbilitySystemComponent.h"
#include "GameplayTags/GP_Tags.h"

AGP_PlayerCharacter::AGP_PlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.2f;
	
	// 초기 속도 세팅
	GetCharacterMovement()->MaxWalkSpeed = GetScaledNormalWalkSpeed(); 
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	
	// 물리 제동 마찰력 (추후 블루프린트에서 제어하여 슬라이딩 거리를 조절합니다)
	GetCharacterMovement()->BrakingDecelerationWalking = 1000.f; 
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.f;

	UEFNSourceMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("UEFNSourceMesh"));
	UEFNSourceMesh->SetupAttachment(GetCapsuleComponent());
	UEFNSourceMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	UEFNSourceMesh->SetGenerateOverlapEvents(false);
	UEFNSourceMesh->SetHiddenInGame(true);
	UEFNSourceMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	UEFNSourceMesh->SetRelativeLocation(FVector::ZeroVector);
	UEFNSourceMesh->SetRelativeRotation(FRotator::ZeroRotator);

	GetMesh()->SetupAttachment(UEFNSourceMesh);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>("CameraBoom");
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = 380.0f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 12.0f;
	CameraBoom->bEnableCameraRotationLag = true;
	CameraBoom->CameraRotationLagSpeed = 15.0f;
	CameraBoom->SocketOffset = FVector(0.0f, 50.0f, 20.0f);
	CameraBoom->TargetOffset = FVector(0.0f, 0.0f, 0.0f);

	FollowCamera = CreateDefaultSubobject<UCameraComponent>("FollowCamera");
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	
	// 태그 추가 함수 추가후 호출 예정지
}

void AGP_PlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	ApplyMovementSpeedFromAnimationSet();
	ApplyRetargetVisualScaleFromAnimationSet();
}

void AGP_PlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 1. 소스 메시의 루트 모션 소비 (메시 이탈 방지를 위해 매 틱 무조건 Consume)
	FRotator RootMotionDeltaRot = FRotator::ZeroRotator;
	if (UEFNSourceMesh && UEFNSourceMesh->IsPlayingRootMotion())
	{
		FRootMotionMovementParams RootMotion = UEFNSourceMesh->ConsumeRootMotion();
		if (RootMotion.bHasRootMotion)
		{
			RootMotionDeltaRot = RootMotion.GetRootMotionTransform().GetRotation().Rotator();
		}
	}

	const float SpeedSq = GetVelocity().SizeSquared2D();
	const bool bIsStatic = SpeedSq < 225.f; // Speed < 15.f

	if (bIsStatic)
	{
		// 1. 제자리 대기 및 회전 시: 모션 매칭 애니메이션이 주도하는 순수 루트 모션 회전량만 100% 반영
		//    C++ 측의 인위적인 수동 RInterpTo 보간 개입을 전면 차단하여 발 미끄러짐과 튕김 현상을 완벽히 근절하고,
		//    최종 1:1 정렬 수렴 처리는 Pose Search Schema(PSS)의 Heading/Yaw 가중치 튜닝을 통해 모션 매칭 엔진이 자체 해결하도록 위임합니다.
		if (!RootMotionDeltaRot.IsZero())
		{
			AddActorWorldRotation(RootMotionDeltaRot);
		}
	}

	UpdateConditionalMaxAcceleration(DeltaSeconds);
}

void AGP_PlayerCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
}

// ==========================================
// GAS 및 초기화 로직
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
	GetAbilitySystemComponent()->RegisterGameplayTagEvent(GPTags::State::Movement::Sprinting, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::OnSprintingTagChanged);
	GetAbilitySystemComponent()->RegisterGameplayTagEvent(GPTags::State::Status::Fixed, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::OnFixedTagChanged);
	
	OnASCInitialized.Broadcast(GetAbilitySystemComponent(), GetAttributeSet());
	GiveStartupAbilities();
	InitializeAttributes();
}

void AGP_PlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	if (!IsValid(GetAbilitySystemComponent())) return;
	
	GetAbilitySystemComponent()->InitAbilityActorInfo(GetPlayerState(), this);
	
	// 클라이언트 환경 Sprinting 태그 리스너 바인딩
	GetAbilitySystemComponent()->RegisterGameplayTagEvent(GPTags::State::Movement::Sprinting, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::OnSprintingTagChanged);
	GetAbilitySystemComponent()->RegisterGameplayTagEvent(GPTags::State::Status::Fixed, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::OnFixedTagChanged);

	OnASCInitialized.Broadcast(GetAbilitySystemComponent(), GetAttributeSet());
}




void AGP_PlayerCharacter::AddMovementInput(FVector WorldDirection, float ScaleValue, bool bForce)
{
	// 기존 IsSprintExitControlLocked() 대신, ASC에서 직접 Fixed 태그만 검사
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	
	if (!bForce && ASC && ASC->HasMatchingGameplayTag(GPTags::State::Status::Fixed))
	{
		return; 
	}
	
	Super::AddMovementInput(WorldDirection, ScaleValue, bForce);
}

void AGP_PlayerCharacter::ToggleSprinting()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return;
	
	// Sprinting 토글을 위한 태그 요청 (어빌리티 동작을 가정)
	FGameplayTagContainer SprintTag;
	SprintTag.AddTag(GPTags::State::Movement::Sprinting); 
	
	if (IsSprinting())
	{
		// 달리기 중이라면 어빌리티/태그 강제 취소
		ASC->CancelAbilities(&SprintTag);
	}
	else
	{
		// 걷기 중이라면 달리기 활성화 시도
		ASC->TryActivateAbilitiesByTag(SprintTag);
	}
}

void AGP_PlayerCharacter::StartSprinting()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || IsSprinting()) return;

	FGameplayTagContainer SprintTag;
	SprintTag.AddTag(GPTags::State::Movement::Sprinting); 
	ASC->TryActivateAbilitiesByTag(SprintTag);
}

void AGP_PlayerCharacter::StopSprinting()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !IsSprinting()) return;

	FGameplayTagContainer SprintTag;
	SprintTag.AddTag(GPTags::State::Movement::Sprinting); 
	ASC->CancelAbilities(&SprintTag);
}

bool AGP_PlayerCharacter::IsSprinting() const
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	return ASC ? ASC->HasMatchingGameplayTag(GPTags::State::Movement::Sprinting) : false;
}

bool AGP_PlayerCharacter::IsDashing() const
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	return ASC ? ASC->HasMatchingGameplayTag(GPTags::State::Movement::Dash) : false;
}

float AGP_PlayerCharacter::GetMovementSpeedScaleRatio() const
{
	return FMath::Max(GetActiveMovementSpeedProfile().MovementSpeedScaleRatio * GASMovementSpeedScaleRatioMultiplier, 0.01f);
}

float AGP_PlayerCharacter::GetScaledNormalWalkSpeed() const
{
	return GetActiveMovementSpeedProfile().NormalForwardSpeed * GetMovementSpeedScaleRatio() * FMath::Max(GASMovementSpeedMultiplier, 0.01f);
}

float AGP_PlayerCharacter::GetScaledSprintSpeed() const
{
	return GetActiveMovementSpeedProfile().SprintForwardSpeed * GetMovementSpeedScaleRatio() * FMath::Max(GASMovementSpeedMultiplier, 0.01f);
}

float AGP_PlayerCharacter::ResolveDirectionalMoveSpeed(const FVector2D& MoveInput, bool bSprinting) const
{
	const FGPDirectionalMovementSpeedProfile& Profile = GetActiveMovementSpeedProfile();
	const FVector2D Input = MoveInput.GetSafeNormal();
	const float ForwardAmount = Input.Y;
	const float AbsForward = FMath::Abs(Input.Y);
	const float AbsRight = FMath::Abs(Input.X);

	float BaseSpeed = Profile.NormalForwardSpeed;
	if (FMath::IsNearlyZero(ForwardAmount) && AbsRight > 0.f)
	{
		BaseSpeed = bSprinting ? Profile.SprintSideSpeed : Profile.NormalSideSpeed;
	}
	else if (ForwardAmount > 0.f)
	{
		BaseSpeed = bSprinting ? Profile.SprintForwardSpeed : Profile.NormalForwardSpeed;
	}
	else if (AbsRight > AbsForward)
	{
		BaseSpeed = bSprinting ? Profile.SprintSideSpeed : Profile.NormalSideSpeed;
	}
	else
	{
		BaseSpeed = bSprinting ? Profile.SprintBackSpeed : Profile.NormalBackSpeed;
	}

	return BaseSpeed * GetMovementSpeedScaleRatio() * FMath::Max(GASMovementSpeedMultiplier, 0.01f);
}

void AGP_PlayerCharacter::SetGASMovementSpeedMultiplier(float NewMultiplier)
{
	GASMovementSpeedMultiplier = FMath::Max(NewMultiplier, 0.01f);
	RefreshCurrentMaxWalkSpeed();
}

void AGP_PlayerCharacter::SetGASMovementSpeedScaleRatioMultiplier(float NewMultiplier)
{
	GASMovementSpeedScaleRatioMultiplier = FMath::Max(NewMultiplier, 0.01f);
	RefreshCurrentMaxWalkSpeed();
}

void AGP_PlayerCharacter::SetMovementSpeedProfileOverride(const FGPDirectionalMovementSpeedProfile& NewProfile)
{
	MovementSpeedProfileOverride = NewProfile;
	bOverrideMovementSpeedProfile = true;
	RefreshCurrentMaxWalkSpeed();
}

void AGP_PlayerCharacter::ClearMovementSpeedProfileOverride()
{
	bOverrideMovementSpeedProfile = false;
	RefreshCurrentMaxWalkSpeed();
}

void AGP_PlayerCharacter::ApplyMovementSpeedFromAnimationSet()
{
	if (AnimationSet)
	{
		MovementSpeedProfile = AnimationSet->MovementSpeedProfile;
	}
	RefreshCurrentMaxWalkSpeed();
}

void AGP_PlayerCharacter::RefreshCurrentMaxWalkSpeed()
{
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = IsSprinting() ? GetScaledSprintSpeed() : GetScaledNormalWalkSpeed();
	}
}

const FGPDirectionalMovementSpeedProfile& AGP_PlayerCharacter::GetActiveMovementSpeedProfile() const
{
	return bOverrideMovementSpeedProfile ? MovementSpeedProfileOverride : MovementSpeedProfile;
}

void AGP_PlayerCharacter::UpdateAnimationSet()
{
	Super::UpdateAnimationSet();
	ApplyMovementSpeedFromAnimationSet();
	ApplyRetargetVisualScaleFromAnimationSet();
}

void AGP_PlayerCharacter::ApplyRetargetVisualScaleFromAnimationSet()
{
	if (!AnimationSet)
	{
		return;
	}

	const FGPRetargetVisualScaleProfile& VisualScaleProfile = AnimationSet->RetargetVisualScaleProfile;
	const float UEFNSourceScale = FMath::Max(VisualScaleProfile.UEFNSourceMeshScale, 0.01f);
	if (UEFNSourceMesh)
	{
		UEFNSourceMesh->SetRelativeScale3D(FVector(UEFNSourceScale));
	}

	// CharacterMesh0 is attached under UEFNSourceMesh, so compensate the child-relative
	// value to keep the authored PDA scale independent from the source mesh scale.
	const float CharacterMeshScale = FMath::Max(VisualScaleProfile.CharacterMeshScale, 0.01f);
	GetMesh()->SetRelativeScale3D(FVector(CharacterMeshScale / UEFNSourceScale));
}

bool AGP_PlayerCharacter::TryPerformDash()
{
	if (!GetAbilitySystemComponent()) return false;
    
	if (GetCharacterMovement()->IsFalling()) return false;

	FGameplayTagContainer DashTag;
	DashTag.AddTag(GPTags::Ability::Movement::Dash);
    
	return GetAbilitySystemComponent()->TryActivateAbilitiesByTag(DashTag);
}


UBlendSpace* AGP_PlayerCharacter::GetLocomotionBlendSpace() const { return AnimationSet ? AnimationSet->LocomotionBlendSpace : nullptr; }
UAnimSequenceBase* AGP_PlayerCharacter::GetJumpLoopAnimation() const { return AnimationSet ? AnimationSet->JumpLoopAnimation : nullptr; }


void AGP_PlayerCharacter::OnSprintingTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	// PlayerController smooths MaxWalkSpeed toward the sprint/non-sprint target.
}

void AGP_PlayerCharacter::OnFixedTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = false;
		MoveComp->bUseControllerDesiredRotation = false;
	}
}

#include "AbilitySystem/Abilities/GP_SkillData.h"

void AGP_PlayerCharacter::EquipSkill(UGP_SkillData* NewSkillData, FGameplayTag SlotTag, bool bIgnoreRestrictions)
{
	if (!NewSkillData || !NewSkillData->AbilityClass) return;

	// 1. 슬롯 제한 체크 (로그라이크 예외 지원)
	if (!bIgnoreRestrictions)
	{
		// 데이터 에셋에 허용 슬롯이 정의되어 있는데, 현재 요청한 슬롯이 그 안에 없다면 거부
		if (!NewSkillData->SupportedSlotTags.IsEmpty() && !NewSkillData->SupportedSlotTags.HasTag(SlotTag))
		{
			UE_LOG(LogTemp, Warning, TEXT("Skill '%s' is not compatible with slot %s"), *NewSkillData->SkillName.ToString(), *SlotTag.ToString());
			return;
		}
	}

	EquipSkillByClass(SlotTag, NewSkillData->AbilityClass);

	// 교환 성공 알림 방송 (UI 연동용)
	if (OnSkillEquipped.IsBound())
	{
		OnSkillEquipped.Broadcast(SlotTag, NewSkillData);
	}
}

void AGP_PlayerCharacter::EquipSkillByClass(FGameplayTag SlotTag, TSubclassOf<UGameplayAbility> NewAbilityClass)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !NewAbilityClass || !HasAuthority()) return;

	// 1. 기존 해당 슬롯에 있던 어빌리티 제거 (중복 방지)
	const TArray<FGameplayAbilitySpec>& Specs = ASC->GetActivatableAbilities();
	TArray<FGameplayAbilitySpecHandle> HandlesToRemove;
	
	for (const FGameplayAbilitySpec& Spec : Specs)
	{
		if (Spec.GetDynamicSpecSourceTags().HasTagExact(SlotTag))
		{
			HandlesToRemove.Add(Spec.Handle);
		}
	}

	for (const FGameplayAbilitySpecHandle& Handle : HandlesToRemove)
	{
		ASC->ClearAbility(Handle);
	}

	// 2. 새 어빌리티 부여
	FGameplayAbilitySpec NewSpec(NewAbilityClass);
	NewSpec.GetDynamicSpecSourceTags().AddTag(SlotTag); 
    
	ASC->GiveAbility(NewSpec);
}

void AGP_PlayerCharacter::UpdateConditionalMaxAcceleration(float DeltaSeconds)
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp) return;

	if (bIsStartAccelerationClamped)
	{
		StartAccelerationClampElapsed += DeltaSeconds;

		if (ShouldReleaseAccelerationClamp())
		{
			RestoreNormalMaxAcceleration();
		}
		else
		{
			MoveComp->MaxAcceleration = StartClampMaxAcceleration;
		}
	}
	else
	{
		if (ShouldStartAccelerationClamp())
		{
			NormalMaxAcceleration = MoveComp->MaxAcceleration;
			MoveComp->MaxAcceleration = StartClampMaxAcceleration;
			bIsStartAccelerationClamped = true;
			StartAccelerationClampElapsed = 0.0f;
		}
	}
}

bool AGP_PlayerCharacter::ShouldStartAccelerationClamp() const
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp) return false;

	// 1. 지상 상태
	if (MoveComp->IsFalling()) return false;

	// 2. 정지에 가까운 상태 (수평 속도 < 15.0f)
	if (GetVelocity().SizeSquared2D() >= 225.0f) return false;

	// 3. 이동 입력 발생 (가속 입력 > 0.0f)
	if (MoveComp->GetCurrentAcceleration().Size() <= 0.0f) return false;

	// 4. Sprint 아님
	if (IsSprinting()) return false;

	// 5. Dash 아님
	if (IsDashing()) return false;

	// 6. LockOn 아님
	if (bIsLockOn) return false;

	// 7. GAS 태그 예외 필터링 (Aiming, Combat, Fixed 상태 아닐 것)
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (ASC)
	{
		FGameplayTag AimingTag = FGameplayTag::RequestGameplayTag(FName("State.Movement.Aiming"), false);
		FGameplayTag CombatTag = FGameplayTag::RequestGameplayTag(FName("State.Status.Combat"), false);
		FGameplayTag FixedTag = GPTags::State::Status::Fixed;

		if (AimingTag.IsValid() && ASC->HasMatchingGameplayTag(AimingTag)) return false;
		if (CombatTag.IsValid() && ASC->HasMatchingGameplayTag(CombatTag)) return false;
		if (FixedTag.IsValid() && ASC->HasMatchingGameplayTag(FixedTag)) return false;
	}

	return true;
}

bool AGP_PlayerCharacter::ShouldReleaseAccelerationClamp() const
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp) return true;

	// 1. 공중 상태 진입 (체공)
	if (MoveComp->IsFalling()) return true;

	// 2. 클램프 타이머 경과 (0.5초 초과)
	if (StartAccelerationClampElapsed >= StartClampMaxDuration) return true;

	// 3. 임계 속도 돌파 (수평 속도 >= 250.0f)
	if (GetVelocity().SizeSquared2D() >= (StartClampReleaseSpeed * StartClampReleaseSpeed)) return true;

	// 4. 이동 입력 사라짐
	if (MoveComp->GetCurrentAcceleration().Size() == 0.0f) return true;

	// 5. Sprint 상태 전환
	if (IsSprinting()) return true;

	// 6. Dash 상태 전환
	if (IsDashing()) return true;

	// 7. LockOn 상태 전환
	if (bIsLockOn) return true;

	// 8. GAS 전투/조준/고정 예외 인터럽트 발생
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (ASC)
	{
		FGameplayTag AimingTag = FGameplayTag::RequestGameplayTag(FName("State.Movement.Aiming"), false);
		FGameplayTag CombatTag = FGameplayTag::RequestGameplayTag(FName("State.Status.Combat"), false);
		FGameplayTag FixedTag = GPTags::State::Status::Fixed;

		if (AimingTag.IsValid() && ASC->HasMatchingGameplayTag(AimingTag)) return true;
		if (CombatTag.IsValid() && ASC->HasMatchingGameplayTag(CombatTag)) return true;
		if (FixedTag.IsValid() && ASC->HasMatchingGameplayTag(FixedTag)) return true;
	}

	return false;
}

void AGP_PlayerCharacter::RestoreNormalMaxAcceleration()
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (MoveComp)
	{
		MoveComp->MaxAcceleration = NormalMaxAcceleration;
	}
	bIsStartAccelerationClamped = false;
	StartAccelerationClampElapsed = 0.0f;
}
