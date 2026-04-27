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
#include "GameplayTags/GP_Tags.h"

AGP_PlayerCharacter::AGP_PlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.2f;
	
	// 초기 속도 세팅
	GetCharacterMovement()->MaxWalkSpeed = NormalWalkSpeed; 
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	
	// 물리 제동 마찰력 (추후 블루프린트에서 제어하여 슬라이딩 거리를 조절합니다)
	GetCharacterMovement()->BrakingDecelerationWalking = 1000.f; 
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.f;

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
}

void AGP_PlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
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

	OnASCInitialized.Broadcast(GetAbilitySystemComponent(), GetAttributeSet());
}




void AGP_PlayerCharacter::AddMovementInput(FVector WorldDirection, float ScaleValue, bool bForce)
{
	if (!bForce && IsSprintExitControlLocked())return; 
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

bool AGP_PlayerCharacter::IsPrimaryAttacking() const
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	return ASC ? ASC->HasMatchingGameplayTag(GPTags::Ability::Skill::Primary) : false;
}

bool AGP_PlayerCharacter::IsSprintExitControlLocked() const
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	return ASC ? ASC->HasMatchingGameplayTag(GPTags::State::Status::Fixed) : false; 
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

void AGP_PlayerCharacter::ApplySprintStopLock(float LockTime)
{
	// TODO (GAS): GPTags::Status::Fixed 태그를 LockTime 만큼 부여하는 Gameplay Effect(GE_SprintStopLock)를 여기에 적용해야 함
}

void AGP_PlayerCharacter::OnSprintingTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	// ASC 델리게이트를 통해 Sprint 태그 개수가 변동될 때만 한 번씩 속도를 조절
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = (NewCount > 0) ? SprintSpeed : NormalWalkSpeed;
	}
}

void AGP_PlayerCharacter::EquipSkill(FGameplayTag SlotTag, TSubclassOf<UGameplayAbility> NewAbilityClass)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !NewAbilityClass) return;

	// 1. 기존 해당 슬롯에 있던 어빌리티 제거 (중복 방지)
	// 반환 타입 변경 적용 (const TArray)
	const TArray<FGameplayAbilitySpec>& Specs = ASC->GetActivatableAbilities();
	
	// 순회 중에 배열의 요소를 삭제(ClearAbility)하면 에러가 날 수 있으므로 Handle을 먼저 수집합니다.
	TArray<FGameplayAbilitySpecHandle> HandlesToRemove;
	
	for (const FGameplayAbilitySpec& Spec : Specs)
	{
		// Deprecated 경고 해결: DynamicAbilityTags 대신 GetDynamicSpecSourceTags() 사용
		if (Spec.GetDynamicSpecSourceTags().HasTagExact(SlotTag))
		{
			HandlesToRemove.Add(Spec.Handle);
		}
	}

	// 수집된 Handle들을 일괄 삭제
	for (const FGameplayAbilitySpecHandle& Handle : HandlesToRemove)
	{
		ASC->ClearAbility(Handle);
	}

	// 2. 새 어빌리티 부여
	FGameplayAbilitySpec NewSpec(NewAbilityClass);
	
	// Deprecated 경고 해결: 여기도 최신 API 적용
	NewSpec.GetDynamicSpecSourceTags().AddTag(SlotTag); 
    
	ASC->GiveAbility(NewSpec);
}
