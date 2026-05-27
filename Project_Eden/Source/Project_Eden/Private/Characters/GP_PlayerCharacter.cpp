#include "Characters/GP_PlayerCharacter.h"
#include "Actors/GP_WhiteVoidSetActor.h"
#include "Actors/GP_WhiteVoidSetComponent.h"
#include "Animation/GP_CharacterAnimInstance.h"
#include "Animation/AnimInstance.h"
#include "Animation/GP_CharacterAnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/BlendSpace.h"
#include "Animation/AnimTypes.h"
#include "Animation/PDA_CharacterAnimationSet.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "CharacterTrajectoryComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Player/GP_PlayerState.h"
#include "AbilitySystemComponent.h"
#include "GameplayTags/GP_Tags.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"

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

	WhiteVoidSetClass = AGP_WhiteVoidSetActor::StaticClass();
	
	// 태그 추가 함수 추가후 호출 예정지
}

void AGP_PlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	ApplyMovementSpeedFromAnimationSet();
	ApplyRetargetVisualScaleFromAnimationSet();
	if (bAutoSpawnWhiteVoidSet)
	{
		EnsureWhiteVoidSetExists();
	}
}

void AGP_PlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 1. 소스 메시의 루트 모션 소비 (메시 이탈 방지를 위해 매 틱 무조건 Consume)
	FRotator RootMotionDeltaRot = FRotator::ZeroRotator;
	FVector RootMotionDeltaTranslation = FVector::ZeroVector;
	LastUEFNSourceRootMotionVelocity = FVector::ZeroVector;

	const bool bFallbackMontageActive = IsPlayingUEFNSourceFallbackMontage();
	if (bApplyUEFNSourceFallbackRootMotion && !bFallbackMontageActive)
	{
		bApplyUEFNSourceFallbackRootMotion = false;
		ActiveUEFNSourceFallbackMontage = nullptr;
	}
	if (UEFNSourceMesh && UEFNSourceMesh->IsPlayingRootMotion())
	{
		FRootMotionMovementParams RootMotion = UEFNSourceMesh->ConsumeRootMotion();
		if (RootMotion.bHasRootMotion)
		{
			const FTransform RootMotionTransform = RootMotion.GetRootMotionTransform();
			RootMotionDeltaRot = RootMotionTransform.GetRotation().Rotator();
			RootMotionDeltaTranslation = RootMotionTransform.GetTranslation();
		}
	}

	const float SpeedSq = GetVelocity().SizeSquared2D();
	const bool bIsStatic = SpeedSq < 225.f; // Speed < 15.f

	if (bApplyUEFNSourceFallbackRootMotion && bFallbackMontageActive)
	{
		UCharacterMovementComponent* MoveComp = GetCharacterMovement();
		if (MoveComp && MoveComp->UpdatedComponent && (!RootMotionDeltaTranslation.IsNearlyZero() || !RootMotionDeltaRot.IsZero()))
		{
			const float TranslationYawOffset = AnimationSet ? AnimationSet->SourceRootMotionTranslationYawOffset : 0.0f;
			const FVector CorrectedTranslation = FRotator(0.0f, TranslationYawOffset, 0.0f).RotateVector(RootMotionDeltaTranslation) * GetMovementSpeedScaleRatio();
			const FVector WorldDelta = GetActorTransform().TransformVectorNoScale(CorrectedTranslation);
			const FQuat TargetRotation = RootMotionDeltaRot.IsZero()
				? MoveComp->UpdatedComponent->GetComponentQuat()
				: RootMotionDeltaRot.Quaternion() * MoveComp->UpdatedComponent->GetComponentQuat();

			FHitResult MoveHit;
			MoveComp->SafeMoveUpdatedComponent(WorldDelta, TargetRotation, true, MoveHit);

			if (DeltaSeconds > KINDA_SMALL_NUMBER)
			{
				LastUEFNSourceRootMotionVelocity = WorldDelta / DeltaSeconds;
			}
		}
	}
	else if (bIsStatic)
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

UAnimInstance* AGP_PlayerCharacter::GetUEFNSourceAnimInstance() const
{
	return UEFNSourceMesh ? UEFNSourceMesh->GetAnimInstance() : nullptr;
}

float AGP_PlayerCharacter::PlayUEFNSourceFallbackMontage(UAnimMontage* Montage, float PlayRate)
{
	UAnimInstance* SourceAnimInstance = GetUEFNSourceAnimInstance();
	if (!IsValid(SourceAnimInstance) || !IsValid(Montage))
	{
		return 0.0f;
	}

	if (IsValid(ActiveUEFNSourceFallbackMontage))
	{
		SourceAnimInstance->Montage_Stop(0.1f, ActiveUEFNSourceFallbackMontage);
	}

	const float PlayedDuration = SourceAnimInstance->Montage_Play(Montage, PlayRate);
	if (PlayedDuration > 0.0f)
	{
		ActiveUEFNSourceFallbackMontage = Montage;
		bApplyUEFNSourceFallbackRootMotion = true;
		LastUEFNSourceRootMotionVelocity = FVector::ZeroVector;
	}

	return PlayedDuration;
}

bool AGP_PlayerCharacter::IsPlayingUEFNSourceFallbackMontage() const
{
	UAnimInstance* SourceAnimInstance = GetUEFNSourceAnimInstance();
	return IsValid(SourceAnimInstance)
		&& IsValid(ActiveUEFNSourceFallbackMontage)
		&& SourceAnimInstance->Montage_IsPlaying(ActiveUEFNSourceFallbackMontage);
}

void AGP_PlayerCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
}

void AGP_PlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGP_PlayerCharacter, bIsInWhiteVoid);
}

void AGP_PlayerCharacter::StopUEFNSourceFallbackMontage(float BlendOutTime)
{
	UAnimInstance* SourceAnimInstance = GetUEFNSourceAnimInstance();
	if (IsValid(SourceAnimInstance))
	{
		if (IsValid(ActiveUEFNSourceFallbackMontage))
		{
			SourceAnimInstance->Montage_Stop(BlendOutTime, ActiveUEFNSourceFallbackMontage);
		}
		// 방어 코드: 슬롯명 미스매칭이나 몽타주 매칭 어긋남 대비하여 전체 활성 몽타주 강제 정지 처리
		SourceAnimInstance->Montage_Stop(BlendOutTime, nullptr);
	}
	bApplyUEFNSourceFallbackRootMotion = false;
	ActiveUEFNSourceFallbackMontage = nullptr;
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
	PushMovementSpeedScaleRatioToAnimInstances();
}

void AGP_PlayerCharacter::SetMovementSpeedProfileOverride(const FGPDirectionalMovementSpeedProfile& NewProfile)
{
	MovementSpeedProfileOverride = NewProfile;
	bOverrideMovementSpeedProfile = true;
	RefreshCurrentMaxWalkSpeed();
	PushMovementSpeedScaleRatioToAnimInstances();
}

void AGP_PlayerCharacter::ClearMovementSpeedProfileOverride()
{
	bOverrideMovementSpeedProfile = false;
	RefreshCurrentMaxWalkSpeed();
	PushMovementSpeedScaleRatioToAnimInstances();
}

void AGP_PlayerCharacter::ApplyMovementSpeedFromAnimationSet()
{
	if (AnimationSet)
	{
		MovementSpeedProfile = AnimationSet->MovementSpeedProfile;
	}
	RefreshCurrentMaxWalkSpeed();
	PushMovementSpeedScaleRatioToAnimInstances();
}

void AGP_PlayerCharacter::RefreshCurrentMaxWalkSpeed()
{
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = IsSprinting() ? GetScaledSprintSpeed() : GetScaledNormalWalkSpeed();
	}
}

void AGP_PlayerCharacter::PushMovementSpeedScaleRatioToAnimInstances()
{
	const float ScaleRatio = GetMovementSpeedScaleRatio();
	if (UGP_CharacterAnimInstance* AnimInstance = Cast<UGP_CharacterAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		AnimInstance->SetMovementSpeedScaleRatio(ScaleRatio);
	}
	if (UEFNSourceMesh)
	{
		if (UGP_CharacterAnimInstance* SourceAnimInstance = Cast<UGP_CharacterAnimInstance>(UEFNSourceMesh->GetAnimInstance()))
		{
			SourceAnimInstance->SetMovementSpeedScaleRatio(ScaleRatio);
		}
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

void AGP_PlayerCharacter::ToggleWhiteVoid()
{
	SetWhiteVoidState(!bIsInWhiteVoid);
}

void AGP_PlayerCharacter::EnterWhiteVoid()
{
	SetWhiteVoidState(true);
}

void AGP_PlayerCharacter::ExitWhiteVoid()
{
	SetWhiteVoidState(false);
}

void AGP_PlayerCharacter::ServerSetWhiteVoid_Implementation(bool bNewInWhiteVoid)
{
	SetWhiteVoidState(bNewInWhiteVoid);
}

void AGP_PlayerCharacter::SetWhiteVoidState(bool bNewInWhiteVoid)
{
	if (bIsInWhiteVoid == bNewInWhiteVoid)
	{
		return;
	}

	// Standalone/listen server: this path is authoritative immediately.
	// Dedicated server: clients call the RPC below; the server owns the final replicated movement.
	// Owning clients also run the transition locally so camera lag is off on the visible frame.
	if (!HasAuthority())
	{
		ServerSetWhiteVoid(bNewInWhiteVoid);
	}

	PerformWhiteVoidTransition(bNewInWhiteVoid);
}

void AGP_PlayerCharacter::PerformWhiteVoidTransition(bool bNewInWhiteVoid)
{
	if (bNewInWhiteVoid)
	{
		EnsureWhiteVoidSetExists();
	}

	CacheWhiteVoidTransitionState();

	if (CameraBoom && !bPreserveCameraLagStateOnTransition)
	{
		CameraBoom->bEnableCameraLag = false;
		CameraBoom->bEnableCameraRotationLag = false;
	}

	if (bStopMovementOnTransition)
	{
		if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
		{
			MoveComp->StopMovementImmediately();
			MoveComp->Velocity = FVector::ZeroVector;
			MoveComp->ClearAccumulatedForces();
		}
	}

	const FVector TargetLocation = ResolveWhiteVoidTargetLocation(bNewInWhiteVoid);
	const FRotator TargetRotation = GetActorRotation();
	const FVector WorldOffset = TargetLocation - GetActorLocation();

	// SpringArm keeps previous lag targets internally. Shift those cached targets by the
	// same delta as the teleport so existing lag/drag offset is preserved in the new space
	// instead of interpolating from the old world's height.
	if (CameraBoom)
	{
		CameraBoom->ApplyWorldOffset(WorldOffset, false);
	}

	TeleportTo(TargetLocation, TargetRotation, false, true);

	if (AController* OwningController = GetController())
	{
		OwningController->SetControlRotation(StoredWhiteVoidControlRotation);
	}

	if (FollowCamera && bForceCameraCutOnTransition)
	{
		FollowCamera->NotifyCameraCut();
	}
	if (bResetMotionTrajectoryOnTransition)
	{
		ResetMotionTrajectoryAfterWhiteVoidTransition();
	}
	SuppressMotionMatchingForWhiteVoidTransition();
	if (bForceCameraCutOnTransition)
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
		{
			if (PlayerController->PlayerCameraManager)
			{
				PlayerController->PlayerCameraManager->SetGameCameraCutThisFrame();
			}
		}
	}

	bIsInWhiteVoid = bNewInWhiteVoid;

	if (CameraBoom)
	{
		CameraBoom->UpdateComponentToWorld();
	}
	if (FollowCamera)
	{
		FollowCamera->UpdateComponentToWorld();
	}

	if (!bPreserveCameraLagStateOnTransition)
	{
		FTimerHandle RestoreLagTimerHandle;
		GetWorldTimerManager().SetTimer(RestoreLagTimerHandle, this, &ThisClass::RestoreWhiteVoidCameraLag, 0.05f, false);
	}

	if (bDebugWhiteVoidTransition)
	{
		UE_LOG(LogTemp, Log, TEXT("White Void %s: %s -> %s"),
			bNewInWhiteVoid ? TEXT("Enter") : TEXT("Exit"),
			*GetName(),
			*TargetLocation.ToString());
	}
}

void AGP_PlayerCharacter::CacheWhiteVoidTransitionState()
{
	if (CameraBoom)
	{
		bStoredCameraLagEnabled = CameraBoom->bEnableCameraLag;
		bStoredCameraRotationLagEnabled = CameraBoom->bEnableCameraRotationLag;
	}

	if (!bIsInWhiteVoid)
	{
		StoredWhiteVoidOriginLocation = GetActorLocation();
		StoredWhiteVoidOriginRotation = GetActorRotation();
		bHasStoredWhiteVoidOrigin = true;
	}

	if (const AController* OwningController = GetController())
	{
		StoredWhiteVoidControlRotation = OwningController->GetControlRotation();
	}
	else
	{
		StoredWhiteVoidControlRotation = GetActorRotation();
	}
}

void AGP_PlayerCharacter::RestoreWhiteVoidCameraLag()
{
	if (!CameraBoom)
	{
		return;
	}

	CameraBoom->bEnableCameraLag = bStoredCameraLagEnabled;
	CameraBoom->bEnableCameraRotationLag = bStoredCameraRotationLagEnabled;
}

FVector AGP_PlayerCharacter::ResolveWhiteVoidTargetLocation(bool bNewInWhiteVoid) const
{
	if (bNewInWhiteVoid)
	{
		FVector TargetLocation = bUseFixedWhiteVoidEntryLocation
			? WhiteVoidEntryLocation
			: StoredWhiteVoidOriginLocation + WhiteVoidOffset;
		if (const UCapsuleComponent* Capsule = GetCapsuleComponent())
		{
			const float FloorTopZ = (bUseFixedWhiteVoidEntryLocation ? WhiteVoidEntryLocation.Z : StoredWhiteVoidOriginLocation.Z + WhiteVoidOffset.Z)
				+ WhiteVoidFloorTopZOffset;
			TargetLocation.Z = FloorTopZ + Capsule->GetScaledCapsuleHalfHeight();
		}
		return TargetLocation;
	}

	if (bHasStoredWhiteVoidOrigin)
	{
		return StoredWhiteVoidOriginLocation;
	}

	return GetActorLocation() - WhiteVoidOffset;
}

void AGP_PlayerCharacter::EnsureWhiteVoidSetExists()
{
	if (!GetWorld() || !WhiteVoidSetClass)
	{
		return;
	}

	TArray<AActor*> ExistingSets;
	UGameplayStatics::GetAllActorsOfClass(this, WhiteVoidSetClass, ExistingSets);

	AGP_WhiteVoidSetActor* WhiteVoidSet = ExistingSets.Num() > 0 ? Cast<AGP_WhiteVoidSetActor>(ExistingSets[0]) : nullptr;
	if (!WhiteVoidSet)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.Owner = this;
		WhiteVoidSet = GetWorld()->SpawnActor<AGP_WhiteVoidSetActor>(WhiteVoidSetClass, FTransform::Identity, SpawnParams);
	}

	if (WhiteVoidSet && WhiteVoidSet->WhiteVoidSetComponent)
	{
		WhiteVoidSet->WhiteVoidSetComponent->WhiteVoidOffset = bUseFixedWhiteVoidEntryLocation ? WhiteVoidEntryLocation : WhiteVoidOffset;
		WhiteVoidSet->RebuildWhiteVoidSet();
	}
}

void AGP_PlayerCharacter::ResetMotionTrajectoryAfterWhiteVoidTransition()
{
	UActorComponent* TrajectoryComponent = FindComponentByClass<UCharacterTrajectoryComponent>();
	if (!TrajectoryComponent)
	{
		return;
	}

	const FVector CurrentMeshLocation = GetMesh() ? GetMesh()->GetComponentLocation() : GetActorLocation();

	if (FStructProperty* TrajectoryProperty = FindFProperty<FStructProperty>(TrajectoryComponent->GetClass(), TEXT("Trajectory")))
	{
		if (FTransformTrajectory* Trajectory = TrajectoryProperty->ContainerPtrToValuePtr<FTransformTrajectory>(TrajectoryComponent))
		{
			for (FTransformTrajectorySample& Sample : Trajectory->Samples)
			{
				Sample.Position = CurrentMeshLocation;
			}
		}
	}

	if (FArrayProperty* HistoryProperty = FindFProperty<FArrayProperty>(TrajectoryComponent->GetClass(), TEXT("TranslationHistory")))
	{
		FScriptArrayHelper HistoryHelper(HistoryProperty, HistoryProperty->ContainerPtrToValuePtr<void>(TrajectoryComponent));
		if (CastField<FStructProperty>(HistoryProperty->Inner))
		{
			for (int32 Index = 0; Index < HistoryHelper.Num(); ++Index)
			{
				*reinterpret_cast<FVector*>(HistoryHelper.GetRawPtr(Index)) = CurrentMeshLocation;
			}
		}
	}
}

void AGP_PlayerCharacter::SuppressMotionMatchingForWhiteVoidTransition() const
{
	if (WhiteVoidMotionMatchingSuppressDuration <= 0.f)
	{
		return;
	}

	if (UGP_CharacterAnimInstance* AnimInstance = Cast<UGP_CharacterAnimInstance>(GetMesh() ? GetMesh()->GetAnimInstance() : nullptr))
	{
		AnimInstance->SuppressMotionMatchingUpdate(WhiteVoidMotionMatchingSuppressDuration);
	}

	if (UGP_CharacterAnimInstance* SourceAnimInstance = Cast<UGP_CharacterAnimInstance>(GetUEFNSourceAnimInstance()))
	{
		SourceAnimInstance->SuppressMotionMatchingUpdate(WhiteVoidMotionMatchingSuppressDuration);
	}
}


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

	if (!GiveAbilityToSlot(SlotTag, NewSkillData->AbilityClass, NewSkillData)) return;
	// 교환 성공 알림 방송 (UI 연동용)
	if (OnSkillEquipped.IsBound())
	{
		OnSkillEquipped.Broadcast(SlotTag, NewSkillData);
	}
}

void AGP_PlayerCharacter::EquipSkillByClass(FGameplayTag SlotTag, TSubclassOf<UGameplayAbility> NewAbilityClass)
{
	GiveAbilityToSlot(SlotTag, NewAbilityClass);
}

bool AGP_PlayerCharacter::GiveAbilityToSlot(FGameplayTag SlotTag, TSubclassOf<UGameplayAbility> AbilityClass, UObject* SourceObject)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !AbilityClass || !HasAuthority()) return false;

	ClearAbilitySlot(SlotTag);

	FGameplayAbilitySpec NewSpec(AbilityClass, 1, INDEX_NONE, SourceObject);
	NewSpec.GetDynamicSpecSourceTags().AddTag(SlotTag);

	ASC->GiveAbility(NewSpec);
	return true;
}

void AGP_PlayerCharacter::ClearAbilitySlot(FGameplayTag SlotTag)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !HasAuthority()) return;

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
}

void AGP_PlayerCharacter::UpdateConditionalMaxAcceleration(float DeltaSeconds)
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp) return;

	MoveInputReversalGraceTimeRemaining = FMath::Max(0.0f, MoveInputReversalGraceTimeRemaining - DeltaSeconds);

	FVector CurrentMoveInputDirection = MoveComp->GetLastInputVector().GetSafeNormal2D();
	if (CurrentMoveInputDirection.IsNearlyZero())
	{
		CurrentMoveInputDirection = MoveComp->GetCurrentAcceleration().GetSafeNormal2D();
	}

	if (!CurrentMoveInputDirection.IsNearlyZero())
	{
		if (!LastMoveInputDirection.IsNearlyZero() && FVector::DotProduct(LastMoveInputDirection, CurrentMoveInputDirection) < -0.5f)
		{
			MoveInputReversalGraceTimeRemaining = 0.2f;
			if (bIsStartAccelerationClamped)
			{
				RestoreNormalMaxAcceleration();
			}
		}
		LastMoveInputDirection = CurrentMoveInputDirection;
	}

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

	if (MoveInputReversalGraceTimeRemaining > 0.0f) return false;

	const FVector Velocity2D = GetVelocity().GetSafeNormal2D();
	const FVector Acceleration2D = MoveComp->GetCurrentAcceleration().GetSafeNormal2D();
	if (!Velocity2D.IsNearlyZero() && !Acceleration2D.IsNearlyZero() && FVector::DotProduct(Velocity2D, Acceleration2D) < 0.0f)
	{
		return false;
	}

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
