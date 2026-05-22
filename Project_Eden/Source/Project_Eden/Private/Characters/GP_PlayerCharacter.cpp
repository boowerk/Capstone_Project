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
	CameraBoom->TargetArmLength = 600.0f;
	CameraBoom->bUsePawnControlRotation = true;

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
		// AnimInstance 상태 확인
		const UGP_CharacterAnimInstance* AnimInst = Cast<UGP_CharacterAnimInstance>(UEFNSourceMesh->GetAnimInstance());
		const bool bInTurnInPlace = AnimInst && AnimInst->GetShouldTurnInPlace();

		if (bInTurnInPlace)
		{
			// 1. 제자리 회전(Turn In Place) 상태인 동안은 애니메이션 발 디딤 싱크를 위해 
			//    순수하게 애니메이션의 루트 모션 회전량(RootMotionDeltaRot)을 월드 회전에 누적 적용합니다.
			if (!RootMotionDeltaRot.IsZero())
			{
				AddActorWorldRotation(RootMotionDeltaRot);
			}

			// 2. 후반부 카메라 정면 원샷 스냅 정렬 (One-shot Snap to Camera)
			//    턴이 진행되는 0.4초 동안은 오직 순수 루트 모션만으로 발을 디뎌 돌게 만들어 미끄러짐을 완전히 제거하고,
			//    회전이 거의 마무리되는 후반부(0.4초 이후) 시점에만 고속 회전(InterpSpeed = 12.0f)을 적용해
			//    카메라 정면 방향(Control Rotation Yaw)을 향해 1:1로 한 번에 시원하고 칼같이 밀착 정렬을 마칩니다.
			if (AnimInst && AnimInst->GetTimeSinceTurnInPlaceStarted() > 0.4f)
			{
				if (const AController* PC = GetController())
				{
					const float TargetYaw = PC->GetControlRotation().Yaw;
					const float CurrentYaw = GetActorRotation().Yaw;

					const float NewYaw = FMath::RInterpTo(
						FRotator(0.f, CurrentYaw, 0.f), 
						FRotator(0.f, TargetYaw, 0.f), 
						DeltaSeconds, 
						12.0f
					).Yaw;

					SetActorRotation(FRotator(0.f, NewYaw, 0.f));
				}
			}
		}
		else
		{
			// 3. 제자리 대기(Idle) 중이며 턴인플레이스가 구동되지 않을 때는 액터 강제 회전을 100% OFF 합니다!
			//    마우스를 미세하게 돌리는 시점에서는 액터를 절대 물리적으로 돌리지 않고 각도를 완벽 고정하여,
			//    슬라이딩이나 발 끌림 현상(Foot Slippage)을 단 1픽셀도 허용하지 않고 지면에 완벽 접지시킵니다.
			if (!RootMotionDeltaRot.IsZero())
			{
				AddActorWorldRotation(RootMotionDeltaRot);
			}
		}
	}
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
