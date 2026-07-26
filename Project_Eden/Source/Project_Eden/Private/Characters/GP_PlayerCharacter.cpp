#include "Characters/GP_PlayerCharacter.h"

// Engine / Core Headers
#include "AbilitySystemComponent.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/IConsoleManager.h"
#include "UObject/UnrealType.h"
#include "Engine/OverlapResult.h"
#include "EngineUtils.h"
#include "Perception/AISense_Hearing.h"

// Framework / Component Headers
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "NavigationInvokerComponent.h"
#include "NavigationSystem.h"
#include "Player/GP_PlayerController.h"
#include "UObject/ConstructorHelpers.h"
#include "AI/Controllers/EnemyAIController.h"

// Animation Headers
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/BlendSpace.h"
#include "Animation/AnimTypes.h"

// Project Specific Headers
#include "Player/GP_PlayerState.h"
#include "AbilitySystem/GP_AttributeSet.h"
#include "AbilitySystem/GP_AbilitySystemComponent.h"
#include "Actors/GP_WhiteVoidSetActor.h"
#include "Actors/GP_WhiteVoidSetComponent.h"
#include "Animation/GP_CharacterAnimInstance.h"
#include "Animation/PDA_CharacterAnimationSet.h"
#include "CharacterTrajectoryComponent.h"
#include "GameplayTags/GP_Tags.h"
#include "Utils/GP_BlueprintLibrary.h"
#include "Game/GP_GameMode.h"
#include "Game/GP_GameState.h"

static int32 GGPActionInertiaDebug = 0;
static FAutoConsoleVariableRef CVarGPActionInertiaDebug(
	TEXT("gp.ActionInertia.Debug"),
	GGPActionInertiaDebug,
	TEXT("Log action root-motion/fallback inertia samples."));

AGP_PlayerCharacter::AGP_PlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// Party members can begin tens of thousands of centimeters apart. Keep all
	// three player pawns relevant so an eliminated client can spectate a living
	// teammate even when their villages are outside the default net-cull range.
	bAlwaysRelevant = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.2f;
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCharacterMovement()->SetCrouchedHalfHeight(64.0f); // 기본값 40.0f가 너무 작아 앉은키가 극단적으로 작아지므로 64.0f로 적당하게 상향 조정

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
	GetMesh()->AddTickPrerequisiteComponent(UEFNSourceMesh);

	// Reuse the actual mesh/material authored for NS_Big_Sword without keeping
	// its one-shot smoke and spark emitters alive. Each playable skeleton owns a
	// hand_rSocket, so character-specific placement and scale stay in the asset.
	PersistentWeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PersistentWeaponMesh"));
	PersistentWeaponMesh->SetupAttachment(GetMesh(), TEXT("hand_rSocket"));
	PersistentWeaponMesh->SetRelativeLocation(FVector::ZeroVector);
	PersistentWeaponMesh->SetRelativeRotation(FRotator::ZeroRotator);
	PersistentWeaponMesh->SetRelativeScale3D(FVector::OneVector);
	PersistentWeaponMesh->SetAbsolute(false, false, false);
	PersistentWeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PersistentWeaponMesh->SetGenerateOverlapEvents(false);
	PersistentWeaponMesh->SetCanEverAffectNavigation(false);
	PersistentWeaponMesh->SetCastShadow(false);
	PersistentWeaponMesh->SetReceivesDecals(false);
	PersistentWeaponMesh->PrimaryComponentTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> NiagaraSwordMeshFinder(
		TEXT("/Game/Mixed_Magic_VFX_Pack/Static_Meshes/SM_7.SM_7"));
	if (NiagaraSwordMeshFinder.Succeeded())
	{
		PersistentWeaponMesh->SetStaticMesh(NiagaraSwordMeshFinder.Object);
	}

	// The Niagara renderer overrides SM_7's default material with this instance.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> NiagaraSwordMaterialFinder(
		TEXT("/Game/Mixed_Magic_VFX_Pack/Materials/Instance_Materials/MI_Ice_Inst_4.MI_Ice_Inst_4"));
	if (NiagaraSwordMaterialFinder.Succeeded())
	{
		PersistentWeaponMesh->SetMaterial(0, NiagaraSwordMaterialFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> EliminationVFXFinder(
		TEXT("/Game/Niagara/Dissolve_SK/NS_EnemyDeath_Absorb.NS_EnemyDeath_Absorb"));
	if (EliminationVFXFinder.Succeeded())
	{
		EliminationVFX = EliminationVFXFinder.Object;
	}

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>("CameraBoom");
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = NormalCameraArmLength;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 12.0f;
	CameraBoom->bEnableCameraRotationLag = true;
	CameraBoom->CameraRotationLagSpeed = 15.0f;
	CameraBoom->SocketOffset = CameraSocketOffset;
	CameraBoom->TargetOffset = FVector(0.0f, 0.0f, 0.0f);

	FollowCamera = CreateDefaultSubobject<UCameraComponent>("FollowCamera");
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Every authoritative player becomes a navigation seed; the wider removal radius prevents tile churn at the generation boundary.
	NavigationInvoker = CreateDefaultSubobject<UNavigationInvokerComponent>(TEXT("NavigationInvoker"));
	NavigationInvoker->SetGenerationRadii(5000.0f, 7000.0f);

	WhiteVoidSetClass = AGP_WhiteVoidSetActor::StaticClass();

	// 태그 추가 함수 추가후 호출 예정지
}

AGP_PlayerCharacter::~AGP_PlayerCharacter()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RestoreLagTimerHandle);
		World->GetTimerManager().ClearTimer(EliminationRecoveryTimerHandle);
		World->GetTimerManager().ClearTimer(EliminationVisualTimerHandle);
		World->GetTimerManager().ClearTimer(EliminationVFXStopTimerHandle);
	}

	OnActionRootMotionCancelInput.Clear();
	ActionMotionSamples.Empty();
}

void AGP_PlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// A replicated slot can already be present when this pawn enters play. Reapply it
	// after the base animation set has initialized the default MaskMan appearance.
	ApplyPartyVisualSlot();
	ApplyMovementSpeedFromAnimationSet();
	ApplyRetargetVisualScaleFromAnimationSet();
	if (bAutoSpawnWhiteVoidSet)
	{
		EnsureWhiteVoidSetExists();
	}
}

void AGP_PlayerCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->NavAgentProps.bCanCrouch = true;
	}

	if (UEFNSourceMesh && UEFNSourceMesh->GetAttachParent() != GetCapsuleComponent())
	{
		UEFNSourceMesh->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	}

	if (UEFNSourceMesh && GetMesh() && GetMesh()->GetAttachParent() != UEFNSourceMesh)
	{
		GetMesh()->AttachToComponent(UEFNSourceMesh, FAttachmentTransformRules::KeepRelativeTransform);
		GetMesh()->AddTickPrerequisiteComponent(UEFNSourceMesh);
	}
}

void AGP_PlayerCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
}

void AGP_PlayerCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
}

void AGP_PlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (IsEliminated())
	{
		return;
	}

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
			const float RootMotionScaleRatio = GetMovementSpeedScaleRatio() * FMath::Max(FallbackRootMotionDistanceCorrection, 0.01f);
			const FVector CorrectedTranslation = FRotator(0.0f, TranslationYawOffset, 0.0f).RotateVector(RootMotionDeltaTranslation) * RootMotionScaleRatio;
			const FVector WorldDelta = GetActorTransform().TransformVectorNoScale(CorrectedTranslation);
			const FQuat TargetRotation = RootMotionDeltaRot.IsZero()
				? MoveComp->UpdatedComponent->GetComponentQuat()
				: RootMotionDeltaRot.Quaternion() * MoveComp->UpdatedComponent->GetComponentQuat();

			FHitResult MoveHit;
			const FVector LocationBeforeMove = GetActorLocation();
			MoveComp->SafeMoveUpdatedComponent(WorldDelta, TargetRotation, true, MoveHit);

			if (DeltaSeconds > KINDA_SMALL_NUMBER)
			{
				const FVector ActualWorldDelta = GetActorLocation() - LocationBeforeMove;
				LastUEFNSourceRootMotionVelocity = ActualWorldDelta / DeltaSeconds;
				if (!LastUEFNSourceRootMotionVelocity.IsNearlyZero())
				{
					LastNonZeroUEFNSourceRootMotionVelocity = LastUEFNSourceRootMotionVelocity;
				}

				if (GGPActionInertiaDebug != 0)
				{
					UE_LOG(LogTemp, Warning, TEXT("[ActionRM][FallbackTick] Actor=%s Dt=%.4f Scale=%.3f Correction=%.3f RootDelta=%.1f WorldDelta=%.1f ActualDelta=%.1f ActualSpeed=%.1f"),
						*GetName(),
						DeltaSeconds,
						RootMotionScaleRatio,
						FallbackRootMotionDistanceCorrection,
						RootMotionDeltaTranslation.Size2D(),
						WorldDelta.Size2D(),
						ActualWorldDelta.Size2D(),
						LastUEFNSourceRootMotionVelocity.Size2D());
				}
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
	UpdateActionCarryVelocity(DeltaSeconds);
	UpdateActionMotionTracking(DeltaSeconds);
	UpdatePrimaryAttackAutoFacing(DeltaSeconds);
	UpdatePrimaryAttackMovementAssist(DeltaSeconds);
	UpdateCameraMotion(DeltaSeconds);
	UpdateFootstepNoise();
}

void AGP_PlayerCharacter::UpdateFootstepNoise()
{
	if (!bEmitFootstepNoise || !HasAuthority())
	{
		return;
	}

	const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	const UWorld* World = GetWorld();
	if (!IsValid(MovementComponent) || !IsValid(World))
	{
		return;
	}

	const float WorldTimeSeconds = World->GetTimeSeconds();
	if (!FootstepNoisePolicy::ShouldReportNoise(
		FootstepNoiseSettings,
		GetVelocity().Size2D(),
		MovementComponent->IsMovingOnGround(),
		IsSprinting(),
		WorldTimeSeconds,
		LastFootstepNoiseTimeSeconds))
	{
		return;
	}

	const float Loudness = FootstepNoisePolicy::ResolveLoudness(FootstepNoiseSettings, IsSprinting(), bIsCrouched);
	if (Loudness <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	FVector FootstepLocation = GetActorLocation();
	if (const UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		// Emit at floor height so the hearing debug stimulus matches the perceived source of the step.
		FootstepLocation.Z -= Capsule->GetScaledCapsuleHalfHeight();
	}

	UAISense_Hearing::ReportNoiseEvent(
		this,
		FootstepLocation,
		Loudness,
		this,
		FMath::Max(0.0f, FootstepNoiseSettings.MaximumNoiseRange),
		FName(TEXT("Footstep")));
	LastFootstepNoiseTimeSeconds = WorldTimeSeconds;
}

bool AGP_PlayerCharacter::AimPrimaryAttackAtBestTarget(float SearchRadius, float ForwardOffset, float Duration)
{
	AActor* BestTarget = FindBestPrimaryAttackTarget(SearchRadius, ForwardOffset);
	if (!IsValid(BestTarget))
	{
		PrimaryAttackAutoFacingTarget = nullptr;
		PrimaryAttackAutoFacingEndTime = 0.0f;
		TargetActor = nullptr;
		return false;
	}

	TargetActor = BestTarget;
	PrimaryAttackAutoFacingTarget = BestTarget;
	PrimaryAttackAutoFacingEndTime = GetWorld() ? GetWorld()->GetTimeSeconds() + FMath::Max(Duration, 0.0f) : 0.0f;
	UpdatePrimaryAttackAutoFacing(0.0f);
	return true;
}

AActor* AGP_PlayerCharacter::FindBestPrimaryAttackTarget(float SearchRadius, float ForwardOffset) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GP_PrimaryAttackAutoTarget), false);
	QueryParams.AddIgnoredActor(this);

	FCollisionResponseParams ResponseParams;
	ResponseParams.CollisionResponse.SetAllChannels(ECR_Ignore);
	ResponseParams.CollisionResponse.SetResponse(ECC_Pawn, ECR_Block);
	ResponseParams.CollisionResponse.SetResponse(ECC_WorldDynamic, ECR_Block);

	TArray<FOverlapResult> OverlapResults;
	const FVector SearchCenter = GetActorLocation() + GetActorForwardVector() * FMath::Max(ForwardOffset, 0.0f);
	World->OverlapMultiByChannel(
		OverlapResults,
		SearchCenter,
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeSphere(FMath::Max(SearchRadius, 1.0f)),
		QueryParams,
		ResponseParams);

	AActor* BestTarget = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	const FVector Forward2D = GetActorForwardVector().GetSafeNormal2D();
	const FVector ActorLocation = GetActorLocation();

	for (const FOverlapResult& Result : OverlapResults)
	{
		AActor* Candidate = Result.GetActor();
		if (!IsValid(Candidate) || !UGP_BlueprintLibrary::CanApplyCombatEffect(const_cast<AGP_PlayerCharacter*>(this), Candidate))
		{
			continue;
		}

		FVector ToCandidate = Candidate->GetActorLocation() - ActorLocation;
		ToCandidate.Z = 0.0f;
		const float Distance = ToCandidate.Size();
		if (Distance <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const float FacingDot = FVector::DotProduct(Forward2D, ToCandidate / Distance);
		if (FacingDot < -0.35f)
		{
			continue;
		}

		const float Score = Distance - FacingDot * 150.0f;
		if (Score < BestScore)
		{
			BestScore = Score;
			BestTarget = Candidate;
		}
	}

	return BestTarget;
}

void AGP_PlayerCharacter::UpdatePrimaryAttackAutoFacing(float DeltaSeconds)
{
	const UWorld* World = GetWorld();
	const float CurrentTime = World ? World->GetTimeSeconds() : 0.0f;
	if (!IsValid(PrimaryAttackAutoFacingTarget) || (PrimaryAttackAutoFacingEndTime > 0.0f && CurrentTime > PrimaryAttackAutoFacingEndTime))
	{
		PrimaryAttackAutoFacingTarget = nullptr;
		return;
	}

	FVector ToTarget = PrimaryAttackAutoFacingTarget->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.0f;
	if (ToTarget.IsNearlyZero())
	{
		return;
	}

	const FRotator TargetYaw(0.0f, ToTarget.Rotation().Yaw, 0.0f);
	const FRotator NewActorRotation = DeltaSeconds > KINDA_SMALL_NUMBER
		? FMath::RInterpTo(GetActorRotation(), TargetYaw, DeltaSeconds, PrimaryAttackAutoFacingRotationInterpSpeed)
		: TargetYaw;
	SetActorRotation(NewActorRotation);
}

UAnimInstance* AGP_PlayerCharacter::GetUEFNSourceAnimInstance() const
{
	return UEFNSourceMesh ? UEFNSourceMesh->GetAnimInstance() : nullptr;
}

float AGP_PlayerCharacter::PlayUEFNSourceFallbackMontage(UAnimMontage* Montage, float PlayRate, float PreviousMontageBlendOutTime)
{
	UAnimInstance* SourceAnimInstance = GetUEFNSourceAnimInstance();
	if (!IsValid(SourceAnimInstance) || !IsValid(Montage))
	{
		return 0.0f;
	}

	if (IsValid(ActiveUEFNSourceFallbackMontage))
	{
		SourceAnimInstance->Montage_Stop(FMath::Max(PreviousMontageBlendOutTime, 0.0f), ActiveUEFNSourceFallbackMontage);
	}

	const float PlayedDuration = SourceAnimInstance->Montage_Play(Montage, PlayRate);
	if (PlayedDuration > 0.0f)
	{
		ActiveUEFNSourceFallbackMontage = Montage;
		bApplyUEFNSourceFallbackRootMotion = true;
		LastUEFNSourceRootMotionVelocity = FVector::ZeroVector;
		LastNonZeroUEFNSourceRootMotionVelocity = FVector::ZeroVector;
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

void AGP_PlayerCharacter::SetActionRootMotionInputCancelEnabled(bool bEnabled)
{
	UE_LOG(LogTemp, Warning, TEXT("[ActionEndTrace][Character] SetInputCancelEnabled=%d Actor=%s"),
		bEnabled ? 1 : 0,
		*GetName());
	bActionRootMotionInputCancelEnabled = bEnabled;
	if (bActionRootMotionInputCancelEnabled)
	{
		RequestActionRootMotionCancelIfMovementHeld();
	}
}

bool AGP_PlayerCharacter::RequestActionRootMotionCancelIfMovementHeld()
{
	if (!bActionRootMotionInputCancelEnabled
		|| FMath::Abs(LastActionRootMotionCancelMovementScale) <= KINDA_SMALL_NUMBER
		|| LastActionRootMotionCancelMovementDirection.IsNearlyZero())
	{
		return false;
	}

	const UWorld* World = GetWorld();
	const float CurrentTime = World ? World->GetTimeSeconds() : 0.0f;
	if (World && HeldMovementInputCancelGraceTime > 0.0f
		&& CurrentTime - LastActionRootMotionCancelMovementInputTime > HeldMovementInputCancelGraceTime)
	{
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("[ActionEndTrace][Character] HeldMoveCancelBroadcast Actor=%s Scale=%.3f Dir=%s"),
		*GetName(),
		LastActionRootMotionCancelMovementScale,
		*LastActionRootMotionCancelMovementDirection.ToCompactString());

	bActionRootMotionInputCancelEnabled = false;
	bActionRootMotionCancelledByMovementInput = true;
	OnActionRootMotionCancelInput.Broadcast();
	return true;
}

void AGP_PlayerCharacter::ClearActionRootMotionCancelMovementInput()
{
	LastActionRootMotionCancelMovementDirection = FVector::ZeroVector;
	LastActionRootMotionCancelMovementScale = 0.0f;
	LastActionRootMotionCancelMovementInputTime = 0.0f;
}

void AGP_PlayerCharacter::SetActionLowerBodyMotionMatchBlendEnabled(bool bEnabled)
{
	bBlendActionLowerBodyToMotionMatching = bEnabled;
	if (!bBlendActionLowerBodyToMotionMatching)
	{
		ActionLowerBodyMotionMatchBlendTargetAlpha = 1.0f;
	}
}

void AGP_PlayerCharacter::SetActionLowerBodyMotionMatchBlendTargetAlpha(float TargetAlpha)
{
	ActionLowerBodyMotionMatchBlendTargetAlpha = FMath::Clamp(TargetAlpha, 0.0f, 1.0f);
}

void AGP_PlayerCharacter::BeginPrimaryAttackMovementAssist(float SpeedRatio)
{
	bPrimaryAttackMovementAssistEnabled = true;
	PrimaryAttackMovementAssistSpeedRatio = FMath::Clamp(SpeedRatio, 0.0f, 1.0f);
	LastPrimaryAttackMovementLogTime = -1000.0f;
}

void AGP_PlayerCharacter::StopPrimaryAttackMovementAssist()
{
	bPrimaryAttackMovementAssistEnabled = false;
	PrimaryAttackMovementAssistSpeedRatio = 0.0f;
	LastPrimaryAttackMovementLogTime = -1000.0f;
}

void AGP_PlayerCharacter::BeginActionMotionTracking()
{
	bTrackActionMotion = true;
	bBlendActionLowerBodyToMotionMatching = false;
	ActionLowerBodyMotionMatchBlendTargetAlpha = 1.0f;
	bActionRootMotionCancelledByMovementInput = false;
	LastActionMotionSampleLocation = GetActorLocation();
	LastActionMotionSampleTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	ActionMotionEntryVelocity = GetVelocity();
	ActionMotionEntryVelocity.Z = 0.0f;
	ActionMotionCarryVelocity = ActionMotionEntryVelocity;
	LastActionCarryActualDelta = FVector::ZeroVector;
	CurrentActionMotionVelocity = FVector::ZeroVector;
	LastNonZeroActionMotionVelocity = FVector::ZeroVector;
	HeldPostActionAnimVelocity = FVector::ZeroVector;
	HeldPostActionAnimVelocityUntilTime = 0.0f;
	ActionMotionSamples.Reset();

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->Velocity.X = 0.0f;
		MoveComp->Velocity.Y = 0.0f;
	}

	if (GGPActionInertiaDebug != 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ActionRM][Begin] Actor=%s EntrySpeed=%.1f Velocity=%s"),
			*GetName(),
			ActionMotionEntryVelocity.Size2D(),
			*ActionMotionEntryVelocity.ToCompactString());
	}
}

void AGP_PlayerCharacter::StopActionMotionTracking()
{
	UE_LOG(LogTemp, Warning, TEXT("[ActionEndTrace][Character] StopActionMotionTracking Actor=%s Track=%d CarrySpeed=%.1f CurrentSpeed=%.1f"),
		*GetName(),
		bTrackActionMotion ? 1 : 0,
		ActionMotionCarryVelocity.Size2D(),
		CurrentActionMotionVelocity.Size2D());
	bTrackActionMotion = false;
	bBlendActionLowerBodyToMotionMatching = false;
	ActionLowerBodyMotionMatchBlendTargetAlpha = 1.0f;
	bActionRootMotionCancelledByMovementInput = false;
	ActionMotionCarryVelocity = FVector::ZeroVector;
	LastActionCarryActualDelta = FVector::ZeroVector;
	CurrentActionMotionVelocity = FVector::ZeroVector;
	LastNonZeroActionMotionVelocity = FVector::ZeroVector;
	HeldPostActionAnimVelocity = FVector::ZeroVector;
	HeldPostActionAnimVelocityUntilTime = 0.0f;
	ActionMotionSamples.Reset();
}

void AGP_PlayerCharacter::UpdatePrimaryAttackMovementAssist(float DeltaSeconds)
{
	if (!bPrimaryAttackMovementAssistEnabled || DeltaSeconds <= KINDA_SMALL_NUMBER || !ShouldApplyActionInertiaDirectMovement())
	{
		return;
	}

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp || !MoveComp->UpdatedComponent)
	{
		return;
	}

	const UWorld* World = GetWorld();
	const float CurrentTime = World ? World->GetTimeSeconds() : 0.0f;
	const float InputAge = World ? CurrentTime - LastActionRootMotionCancelMovementInputTime : -1.0f;
	const bool bShouldLog = !World || CurrentTime - LastPrimaryAttackMovementLogTime >= 0.1f;
	if (bShouldLog)
	{
		LastPrimaryAttackMovementLogTime = CurrentTime;
		UE_LOG(LogTemp, Warning, TEXT("[PrimaryMove] Actor=%s Mode=%d Falling=%d WalkableFloor=%d OnGround=%d Vel=%s Speed2D=%.1f VelZ=%.1f Accel=%s Accel2D=%.1f MaxWalk=%.1f InputAge=%.3f AssistRatio=%.2f"),
			*GetName(),
			static_cast<int32>(MoveComp->MovementMode),
			MoveComp->IsFalling() ? 1 : 0,
			MoveComp->CurrentFloor.IsWalkableFloor() ? 1 : 0,
			MoveComp->IsMovingOnGround() ? 1 : 0,
			*GetVelocity().ToCompactString(),
			GetVelocity().Size2D(),
			GetVelocity().Z,
			*MoveComp->GetCurrentAcceleration().ToCompactString(),
			MoveComp->GetCurrentAcceleration().Size2D(),
			MoveComp->MaxWalkSpeed,
			InputAge,
			PrimaryAttackMovementAssistSpeedRatio);
	}

	if (MoveComp->MovementMode != MOVE_Walking)
	{
		return;
	}

	if (LastActionRootMotionCancelMovementDirection.IsNearlyZero()
		|| FMath::Abs(LastActionRootMotionCancelMovementScale) <= KINDA_SMALL_NUMBER
		|| (World && InputAge > 0.15f))
	{
		return;
	}

	const FVector AssistDirection = LastActionRootMotionCancelMovementDirection.GetSafeNormal2D();
	const float AssistSpeed = GetScaledNormalWalkSpeed() * PrimaryAttackMovementAssistSpeedRatio * FMath::Clamp(FMath::Abs(LastActionRootMotionCancelMovementScale), 0.0f, 1.0f);
	if (AssistSpeed <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float CurrentForwardSpeed = FVector::DotProduct(GetVelocity(), AssistDirection);
	if (CurrentForwardSpeed >= AssistSpeed * 0.8f)
	{
		return;
	}

	FHitResult MoveHit;
	const FVector BeforeLocation = GetActorLocation();
	MoveComp->SafeMoveUpdatedComponent(AssistDirection * AssistSpeed * DeltaSeconds, MoveComp->UpdatedComponent->GetComponentQuat(), true, MoveHit);
	const FVector AppliedDelta = GetActorLocation() - BeforeLocation;
	if (bShouldLog)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PrimaryMove][Assist] Actor=%s AssistSpeed=%.1f CurrentForward=%.1f Delta=%s DeltaSpeed=%.1f Hit=%d Blocking=%d HitNormal=%s"),
			*GetName(),
			AssistSpeed,
			CurrentForwardSpeed,
			*AppliedDelta.ToCompactString(),
			DeltaSeconds > KINDA_SMALL_NUMBER ? AppliedDelta.Size2D() / DeltaSeconds : 0.0f,
			MoveHit.bBlockingHit ? 1 : 0,
			MoveHit.IsValidBlockingHit() ? 1 : 0,
			*MoveHit.Normal.ToCompactString());
	}
}

void AGP_PlayerCharacter::UpdateActionCarryVelocity(float DeltaSeconds)
{
	LastActionCarryActualDelta = FVector::ZeroVector;

	if (!bTrackActionMotion || !bCarryEntryVelocityDuringActionRootMotion || DeltaSeconds < (1.0f / 240.0f) || !ShouldApplyActionInertiaDirectMovement())
	{
		return;
	}

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp || !MoveComp->UpdatedComponent || ActionMotionCarryVelocity.IsNearlyZero())
	{
		return;
	}

	const FVector CarryDelta = ActionMotionCarryVelocity * DeltaSeconds;
	const FVector LocationBeforeMove = GetActorLocation();

	FHitResult MoveHit;
	MoveComp->SafeMoveUpdatedComponent(CarryDelta, MoveComp->UpdatedComponent->GetComponentQuat(), true, MoveHit);
	LastActionCarryActualDelta = GetActorLocation() - LocationBeforeMove;
	LastActionCarryActualDelta.Z = 0.0f;

	const float CarrySpeed = ActionMotionCarryVelocity.Size2D();
	const float CarryDeceleration = MoveComp->GetMaxBrakingDeceleration();
	const float NewCarrySpeed = FMath::Max(CarrySpeed - CarryDeceleration * DeltaSeconds, 0.0f);
	ActionMotionCarryVelocity = NewCarrySpeed > 0.0f
		? ActionMotionCarryVelocity.GetSafeNormal2D() * NewCarrySpeed
		: FVector::ZeroVector;

	if (GGPActionInertiaDebug != 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ActionRM][Carry] Actor=%s Fallback=%d Dt=%.4f EntrySpeed=%.1f CarrySpeed=%.1f CarryDelta=%.1f ActualDelta=%.1f"),
			*GetName(),
			IsPlayingUEFNSourceFallbackMontage() ? 1 : 0,
			DeltaSeconds,
			ActionMotionEntryVelocity.Size2D(),
			CarrySpeed,
			CarryDelta.Size2D(),
			LastActionCarryActualDelta.Size2D());
	}
}

bool AGP_PlayerCharacter::ShouldApplyActionInertiaDirectMovement() const
{
	return GetNetMode() == NM_Standalone;
}

void AGP_PlayerCharacter::UpdateActionMotionTracking(float DeltaSeconds)
{
	if (!bTrackActionMotion || DeltaSeconds < (1.0f / 240.0f))
	{
		return;
	}

	UWorld* World = GetWorld();
	const float CurrentTime = World ? World->GetTimeSeconds() : 0.0f;
	const FVector CurrentLocation = GetActorLocation();
	FVector Delta = CurrentLocation - LastActionMotionSampleLocation;
	Delta.Z = 0.0f;
	CurrentActionMotionVelocity = DeltaSeconds > KINDA_SMALL_NUMBER ? Delta / DeltaSeconds : FVector::ZeroVector;
	if (!CurrentActionMotionVelocity.IsNearlyZero())
	{
		LastNonZeroActionMotionVelocity = CurrentActionMotionVelocity;
		LastNonZeroActionMotionVelocity.Z = 0.0f;
	}
	const FVector EstimatedRootMotionDelta = Delta - LastActionCarryActualDelta;
	LastActionMotionSampleLocation = CurrentLocation;
	LastActionMotionSampleTime = CurrentTime;

	FGPActionMotionSample Sample;
	Sample.Delta = Delta;
	Sample.DeltaSeconds = DeltaSeconds;
	Sample.TimeSeconds = CurrentTime;
	ActionMotionSamples.Add(Sample);

	if (GGPActionInertiaDebug != 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ActionRM][Sample] Actor=%s Fallback=%d Dt=%.4f TotalDelta=%.1f TotalSpeed=%.1f RMDelta=%.1f RMSpeed=%.1f CarryDelta=%.1f CarrySpeed=%.1f Samples=%d"),
			*GetName(),
			IsPlayingUEFNSourceFallbackMontage() ? 1 : 0,
			DeltaSeconds,
			Delta.Size2D(),
			Delta.Size2D() / DeltaSeconds,
			EstimatedRootMotionDelta.Size2D(),
			EstimatedRootMotionDelta.Size2D() / DeltaSeconds,
			LastActionCarryActualDelta.Size2D(),
			LastActionCarryActualDelta.Size2D() / DeltaSeconds,
			ActionMotionSamples.Num());
	}

	const float OldestAllowedTime = CurrentTime - FMath::Max(ActionInertiaSampleWindow, 0.02f);
	while (!ActionMotionSamples.IsEmpty() && ActionMotionSamples[0].TimeSeconds < OldestAllowedTime)
	{
		ActionMotionSamples.RemoveAt(0, 1, EAllowShrinking::No);
	}
}

void AGP_PlayerCharacter::FlushActionMotionTracking()
{
	if (!bTrackActionMotion)
	{
		return;
	}

	UWorld* World = GetWorld();
	const float CurrentTime = World ? World->GetTimeSeconds() : 0.0f;
	const float DeltaSeconds = CurrentTime - LastActionMotionSampleTime;
	if (DeltaSeconds < (1.0f / 120.0f))
	{
		return;
	}

	UpdateActionMotionTracking(DeltaSeconds);
}

FVector AGP_PlayerCharacter::GetCurrentActionInertiaVelocity() const
{
	FVector TotalDelta = FVector::ZeroVector;
	float TotalTime = 0.0f;

	for (const FGPActionMotionSample& Sample : ActionMotionSamples)
	{
		TotalDelta += Sample.Delta;
		TotalTime += Sample.DeltaSeconds;
	}

	if (TotalTime < ActionInertiaMinSampleTime)
	{
		return FVector::ZeroVector;
	}

	FVector SampledVelocity = TotalDelta / TotalTime;
	SampledVelocity.Z = 0.0f;
	return SampledVelocity;
}

FVector AGP_PlayerCharacter::GetActionMotionAnimVelocity() const
{
	if (!CurrentActionMotionVelocity.IsNearlyZero())
	{
		return CurrentActionMotionVelocity;
	}

	if (bTrackActionMotion && !LastNonZeroActionMotionVelocity.IsNearlyZero())
	{
		return LastNonZeroActionMotionVelocity;
	}

	const UWorld* World = GetWorld();
	if (World && World->GetTimeSeconds() <= HeldPostActionAnimVelocityUntilTime)
	{
		return HeldPostActionAnimVelocity;
	}

	return FVector::ZeroVector;
}

bool AGP_PlayerCharacter::IsUsingPostActionAnimVelocity() const
{
	const UWorld* World = GetWorld();
	return World
		&& World->GetTimeSeconds() <= HeldPostActionAnimVelocityUntilTime
		&& !HeldPostActionAnimVelocity.IsNearlyZero();
}

void AGP_PlayerCharacter::ApplyCurrentActionInertia()
{
	FlushActionMotionTracking();
	bTrackActionMotion = false;
	if (!ShouldApplyActionInertiaDirectMovement())
	{
		ActionMotionCarryVelocity = FVector::ZeroVector;
		LastActionCarryActualDelta = FVector::ZeroVector;
		CurrentActionMotionVelocity = FVector::ZeroVector;
		LastNonZeroActionMotionVelocity = FVector::ZeroVector;
		HeldPostActionAnimVelocity = FVector::ZeroVector;
		HeldPostActionAnimVelocityUntilTime = 0.0f;
		bActionRootMotionCancelledByMovementInput = false;
		ActionMotionSamples.Reset();
		return;
	}

	const UWorld* World = GetWorld();
	const float CurrentTime = World ? World->GetTimeSeconds() : 0.0f;
	const bool bHasRecentMoveInput =
		World
		&& FMath::Abs(LastActionRootMotionCancelMovementScale) > KINDA_SMALL_NUMBER
		&& !LastActionRootMotionCancelMovementDirection.IsNearlyZero()
		&& CurrentTime - LastActionRootMotionCancelMovementInputTime <= HeldMovementInputCancelGraceTime;
	const bool bShouldHoldPostActionAnimVelocity =
		bActionRootMotionCancelledByMovementInput || bHasRecentMoveInput;

	if (bShouldHoldPostActionAnimVelocity && PostActionAnimVelocityHoldTime > 0.0f && !LastNonZeroActionMotionVelocity.IsNearlyZero())
	{
		HeldPostActionAnimVelocity = LastNonZeroActionMotionVelocity;
		HeldPostActionAnimVelocity.Z = 0.0f;
		HeldPostActionAnimVelocityUntilTime = World ? CurrentTime + PostActionAnimVelocityHoldTime : 0.0f;
	}
	else
	{
		HeldPostActionAnimVelocity = FVector::ZeroVector;
		HeldPostActionAnimVelocityUntilTime = 0.0f;
	}

	if (!bApplyActionInertiaOnMontageComplete)
	{
		ActionMotionCarryVelocity = FVector::ZeroVector;
		LastActionCarryActualDelta = FVector::ZeroVector;
		CurrentActionMotionVelocity = FVector::ZeroVector;
		LastNonZeroActionMotionVelocity = FVector::ZeroVector;
		bActionRootMotionCancelledByMovementInput = false;
		ActionMotionSamples.Reset();
		return;
	}

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp)
	{
		ActionMotionCarryVelocity = FVector::ZeroVector;
		LastActionCarryActualDelta = FVector::ZeroVector;
		CurrentActionMotionVelocity = FVector::ZeroVector;
		LastNonZeroActionMotionVelocity = FVector::ZeroVector;
		bActionRootMotionCancelledByMovementInput = false;
		ActionMotionSamples.Reset();
		return;
	}

	FVector HandoffVelocity = ActionMotionCarryVelocity;
	HandoffVelocity.Z = 0.0f;
	if (bActionRootMotionCancelledByMovementInput && !LastActionRootMotionCancelMovementDirection.IsNearlyZero())
	{
		const float CarrySpeed = ActionMotionCarryVelocity.Size2D();
		const float EntrySpeed = ActionMotionEntryVelocity.Size2D();
		const float DesiredMoveSpeed = MoveComp->MaxWalkSpeed;
		const float HandoffSeedSpeed = FMath::Max3(CarrySpeed, EntrySpeed, DesiredMoveSpeed);
		HandoffVelocity = LastActionRootMotionCancelMovementDirection.GetSafeNormal2D() * HandoffSeedSpeed;
		const UWorld* HandoffWorld = GetWorld();
		HeldPostActionAnimVelocity = HandoffVelocity;
		HeldPostActionAnimVelocity.Z = 0.0f;
		HeldPostActionAnimVelocityUntilTime = HandoffWorld ? HandoffWorld->GetTimeSeconds() + PostActionAnimVelocityHoldTime : 0.0f;
		UE_LOG(LogTemp, Warning, TEXT("[ActionRM][ApplyHeldInput] Actor=%s CarrySpeed=%.1f EntrySpeed=%.1f DesiredMoveSpeed=%.1f SeedSpeed=%.1f Dir=%s"),
			*GetName(),
			CarrySpeed,
			EntrySpeed,
			DesiredMoveSpeed,
			HandoffSeedSpeed,
			*LastActionRootMotionCancelMovementDirection.ToCompactString());
	}

	const float HandoffSpeed = HandoffVelocity.Size2D();
	if (HandoffSpeed < ActionInertiaMinSpeed)
	{
		if (GGPActionInertiaDebug != 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ActionRM][ApplySkip] Actor=%s EntrySpeed=%.1f HandoffSpeed=%.1f Samples=%d"),
				*GetName(),
				ActionMotionEntryVelocity.Size2D(),
				HandoffSpeed,
				ActionMotionSamples.Num());
		}
		ActionMotionCarryVelocity = FVector::ZeroVector;
		LastActionCarryActualDelta = FVector::ZeroVector;
		CurrentActionMotionVelocity = FVector::ZeroVector;
		LastNonZeroActionMotionVelocity = FVector::ZeroVector;
		bActionRootMotionCancelledByMovementInput = false;
		ActionMotionSamples.Reset();
		return;
	}

	if (ActionInertiaMaxSpeed > 0.0f && HandoffSpeed > ActionInertiaMaxSpeed)
	{
		HandoffVelocity = HandoffVelocity.GetSafeNormal2D() * ActionInertiaMaxSpeed;
	}

	MoveComp->Velocity = FVector(HandoffVelocity.X, HandoffVelocity.Y, MoveComp->Velocity.Z);
	ActionMotionCarryVelocity = FVector::ZeroVector;
	LastActionCarryActualDelta = FVector::ZeroVector;
	CurrentActionMotionVelocity = FVector::ZeroVector;
	LastNonZeroActionMotionVelocity = FVector::ZeroVector;
	bActionRootMotionCancelledByMovementInput = false;
	if (GGPActionInertiaDebug != 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ActionRM][Apply] Actor=%s EntrySpeed=%.1f HandoffSpeed=%.1f Handoff=%s Samples=%d"),
			*GetName(),
			ActionMotionEntryVelocity.Size2D(),
			HandoffVelocity.Size2D(),
			*HandoffVelocity.ToCompactString(),
			ActionMotionSamples.Num());
	}
	ActionMotionSamples.Reset();
}

void AGP_PlayerCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
}

void AGP_PlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AGP_PlayerCharacter, bIsInWhiteVoid, COND_SkipOwner);
	DOREPLIFETIME(AGP_PlayerCharacter, PartyVisualSlot);
}

void AGP_PlayerCharacter::SetPartyVisualSlot(int32 NewPartyVisualSlot)
{
	if (!HasAuthority())
	{
		return;
	}

	PartyVisualSlot = FMath::Clamp(NewPartyVisualSlot, 0, 2);
	ApplyPartyVisualSlot();
	ForceNetUpdate();
}

void AGP_PlayerCharacter::OnRep_PartyVisualSlot()
{
	ApplyPartyVisualSlot();
}

void AGP_PlayerCharacter::ApplyPartyVisualSlot()
{
	USkeletalMesh* PartyMesh = nullptr;
	if (PartyVisualSlot == 1)
	{
		PartyMesh = SecondPlayerSkeletalMesh;
	}
	else if (PartyVisualSlot == 2)
	{
		PartyMesh = ThirdPlayerSkeletalMesh;
	}

	if (!PartyMesh || !GetMesh() || GetMesh()->GetSkeletalMeshAsset() == PartyMesh)
	{
		return;
	}

	// Mirror the AnimationSet setup used by the default MaskMan appearance. Changing only
	// the mesh can leave the component without its runtime-retarget AnimBP on clients.
	GetMesh()->SetSkeletalMeshAsset(PartyMesh);
	if (AnimationSet && AnimationSet->AnimBlueprintClass)
	{
		GetMesh()->SetAnimInstanceClass(AnimationSet->AnimBlueprintClass);
	}
	if (UGP_CharacterAnimInstance* AnimInstance = Cast<UGP_CharacterAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		AnimInstance->SetAnimationSet(AnimationSet);
	}
	ApplyRetargetVisualScaleFromAnimationSet();
}

void AGP_PlayerCharacter::StopUEFNSourceFallbackMontage(float BlendOutTime)
{
	UAnimInstance* SourceAnimInstance = GetUEFNSourceAnimInstance();
	const float SourcePosition = IsValid(SourceAnimInstance) && IsValid(ActiveUEFNSourceFallbackMontage)
		? SourceAnimInstance->Montage_GetPosition(ActiveUEFNSourceFallbackMontage)
		: -1.0f;
	UE_LOG(LogTemp, Warning, TEXT("[ActionEndTrace][Character] StopFallbackMontage Actor=%s Montage=%s Pos=%.3f ApplyRM=%d Blend=%.2f"),
		*GetName(),
		*GetNameSafe(ActiveUEFNSourceFallbackMontage),
		SourcePosition,
		bApplyUEFNSourceFallbackRootMotion ? 1 : 0,
		BlendOutTime);
	if (IsValid(SourceAnimInstance))
	{
		if (IsValid(ActiveUEFNSourceFallbackMontage))
		{
			SourceAnimInstance->Montage_Stop(BlendOutTime, ActiveUEFNSourceFallbackMontage);
		}
		else
		{
			SourceAnimInstance->Montage_Stop(BlendOutTime, nullptr);
		}
	}

	ClearUEFNSourceFallbackMontageState();
}

void AGP_PlayerCharacter::ClearUEFNSourceFallbackMontageState()
{
	bApplyUEFNSourceFallbackRootMotion = false;
	bActionRootMotionInputCancelEnabled = false;
	bBlendActionLowerBodyToMotionMatching = false;
	bActionRootMotionCancelledByMovementInput = false;
	ActiveUEFNSourceFallbackMontage = nullptr;
	LastNonZeroUEFNSourceRootMotionVelocity = FVector::ZeroVector;
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

bool AGP_PlayerCharacter::IsEliminated() const
{
	const AGP_PlayerState* GPPlayerState = GetPlayerState<AGP_PlayerState>();
	return IsValid(GPPlayerState) && GPPlayerState->IsEliminated();
}

void AGP_PlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	if (!IsValid(GetAbilitySystemComponent()) || !HasAuthority()) return;

	GetAbilitySystemComponent()->InitAbilityActorInfo(GetPlayerState(), this);
	GetAbilitySystemComponent()->RegisterGameplayTagEvent(GPTags::State::Movement::Sprinting, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::OnSprintingTagChanged);
	GetAbilitySystemComponent()->RegisterGameplayTagEvent(GPTags::State::Status::Fixed, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::OnFixedTagChanged);

	// Blueprint CDOs serialize BlueprintAssignable invocation lists and can
	// overwrite bindings added by a newer native constructor. Bind this
	// gameplay-critical path explicitly after the ASC has its avatar.
	BindPlayerLifeState(GetAbilitySystemComponent(), GetAttributeSet());
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

	BindPlayerLifeState(GetAbilitySystemComponent(), GetAttributeSet());
	OnASCInitialized.Broadcast(GetAbilitySystemComponent(), GetAttributeSet());
}




void AGP_PlayerCharacter::AddMovementInput(FVector WorldDirection, float ScaleValue, bool bForce)
{
	if (IsEliminated())
	{
		return;
	}

	if (FMath::Abs(ScaleValue) > KINDA_SMALL_NUMBER && !WorldDirection.IsNearlyZero())
	{
		LastActionRootMotionCancelMovementDirection = WorldDirection.GetSafeNormal2D();
		LastActionRootMotionCancelMovementScale = ScaleValue;
		LastActionRootMotionCancelMovementInputTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	}
	else
	{
		ClearActionRootMotionCancelMovementInput();
	}

	// 기존 IsSprintExitControlLocked() 대신, ASC에서 직접 Fixed 태그만 검사
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();

	if (!bForce && ASC && ASC->HasMatchingGameplayTag(GPTags::State::Status::Fixed))
	{
		if (bActionRootMotionInputCancelEnabled && FMath::Abs(ScaleValue) > KINDA_SMALL_NUMBER && !WorldDirection.IsNearlyZero())
		{
			RequestActionRootMotionCancelIfMovementHeld();
			if (!ASC->HasMatchingGameplayTag(GPTags::State::Status::Fixed))
			{
				Super::AddMovementInput(WorldDirection, ScaleValue, bForce);
			}
		}
		return;
	}

	Super::AddMovementInput(WorldDirection, ScaleValue, bForce);
}

void AGP_PlayerCharacter::BindPlayerLifeState(UAbilitySystemComponent* ASC, UAttributeSet* AS)
{
	if (UGP_AttributeSet* GPAttributeSet = Cast<UGP_AttributeSet>(AS))
	{
		GPAttributeSet->OnOutOfHealth.AddUniqueDynamic(this, &ThisClass::HandleOutOfHealth);
	}

	if (AGP_PlayerState* GPPlayerState = GetPlayerState<AGP_PlayerState>())
	{
		GPPlayerState->OnPlayerEliminatedChanged.AddUniqueDynamic(
			this,
			&ThisClass::HandlePlayerEliminatedChanged);
		HandlePlayerEliminatedChanged(GPPlayerState->IsEliminated());
	}
}

void AGP_PlayerCharacter::HandleOutOfHealth(AActor* InstigatorActor, AActor* OutOfHealthTargetActor)
{
	if (!HasAuthority() || OutOfHealthTargetActor != this)
	{
		return;
	}

	AGP_PlayerState* GPPlayerState = GetPlayerState<AGP_PlayerState>();
	UWorld* World = GetWorld();
	if (!IsValid(GPPlayerState) || !IsValid(World) || GPPlayerState->IsEliminated())
	{
		return;
	}

	const float SafeRecoveryDelay = FMath::Max(0.1f, EliminationRecoveryDelay);
	UE_LOG(LogTemp, Log,
		TEXT("[PlayerLife] Player '%s' entered eliminated state; recovery in %.1f seconds."),
		*GPPlayerState->GetPlayerName(),
		SafeRecoveryDelay);
	GPPlayerState->SetEliminated(true, World->GetTimeSeconds() + SafeRecoveryDelay);
	RefreshPartySpectatorTargetsOnServer();

	World->GetTimerManager().SetTimer(
		EliminationRecoveryTimerHandle,
		this,
		&ThisClass::TryRecoverFromElimination,
		SafeRecoveryDelay,
		false);

	if (AGP_GameMode* GPGameMode = World->GetAuthGameMode<AGP_GameMode>())
	{
		GPGameMode->NotifyPlayerEliminated(GPPlayerState);
	}
}

void AGP_PlayerCharacter::HandlePlayerEliminatedChanged(bool bNewEliminated)
{
	ApplyEliminationState(bNewEliminated);
}

void AGP_PlayerCharacter::ApplyEliminationState(bool bNewEliminated)
{
	if (bEliminationStateApplied == bNewEliminated)
	{
		return;
	}

	bEliminationStateApplied = bNewEliminated;
	if (bNewEliminated)
	{
		bMainMeshVisibleBeforeElimination =
			IsValid(GetMesh()) && GetMesh()->IsVisible();
		bSourceMeshVisibleBeforeElimination =
			IsValid(UEFNSourceMesh) && UEFNSourceMesh->IsVisible();
		bWeaponVisibleBeforeElimination =
			IsValid(PersistentWeaponMesh) && PersistentWeaponMesh->IsVisible();

		if (GetNetMode() != NM_DedicatedServer
			&& IsValid(EliminationVFX)
			&& IsValid(GetMesh())
			&& IsValid(GetMesh()->GetSkeletalMeshAsset())
			&& IsValid(GetRootComponent()))
		{
			ActiveEliminationVFXComponent =
				UNiagaraFunctionLibrary::SpawnSystemAttached(
				EliminationVFX,
				GetRootComponent(),
				NAME_None,
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				EAttachLocation::KeepRelativeOffset,
				/*bAutoDestroy=*/true,
				/*bAutoActivate=*/false,
				ENCPoolMethod::None,
				/*bPreCullCheck=*/false);
			if (IsValid(ActiveEliminationVFXComponent))
			{
				// NS_EnemyDeath_Absorb contains a sample Manny only as an
				// editor fallback. Bind the actual replicated player mesh before
				// activation and disable attraction to leave a pure scatter.
				UNiagaraFunctionLibrary::OverrideSystemUserVariableSkeletalMeshComponent(
					ActiveEliminationVFXComponent,
					TEXT("User.SourceMesh"),
					GetMesh());
				ActiveEliminationVFXComponent->SetVariablePosition(
					TEXT("User.AbsorbTargetPosition"),
					GetMesh()->Bounds.Origin);
				ActiveEliminationVFXComponent->SetVariableFloat(
					TEXT("User.AbsorbStrength"),
					0.0f);
				ActiveEliminationVFXComponent->SetVariableFloat(
					TEXT("User.AbsorbKillRadius"),
					0.0f);
				ActiveEliminationVFXComponent->SetVariableVec3(
					TEXT("User.FallGravity"),
					FVector(0.0f, 0.0f, -160.0f));
				ActiveEliminationVFXComponent->SetVariableFloat(
					TEXT("User.AbsorbDrag"),
					1.4f);
				ActiveEliminationVFXComponent->Activate(true);

				EliminationVFXStartTime = GetWorld()->GetTimeSeconds();
				EliminationVFXEmissionFrameCount = 0;
				EliminationVFXStopTimerHandle =
					GetWorldTimerManager().SetTimerForNextTick(
						FTimerDelegate::CreateUObject(
							this,
							&ThisClass::TryStopEliminationVFXEmission));
			}
		}

		GetWorldTimerManager().SetTimer(
			EliminationVisualTimerHandle,
			FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				if (!IsEliminated())
				{
					return;
				}

				if (USkeletalMeshComponent* MainMesh = GetMesh())
				{
					MainMesh->SetVisibility(false, /*bPropagateToChildren=*/false);
				}
				if (IsValid(UEFNSourceMesh))
				{
					UEFNSourceMesh->SetVisibility(false, /*bPropagateToChildren=*/false);
				}
				if (IsValid(PersistentWeaponMesh))
				{
					PersistentWeaponMesh->SetVisibility(false);
				}
			}),
			FMath::Max(0.01f, EliminationMeshHideDelay),
			false);

		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
		{
			ASC->CancelAllAbilities();
		}

		StopSprinting();
		StopJumping();
		UnCrouch();
		StopPrimaryAttackMovementAssist();
		StopActionMotionTracking();
		StopUEFNSourceFallbackMontage(0.15f);
		PrimaryAttackAutoFacingTarget = nullptr;
		TargetActor = nullptr;

		if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
		{
			MoveComp->StopMovementImmediately();
			MoveComp->DisableMovement();
		}

		SetCanBeDamaged(false);
		SetActorEnableCollision(false);
		RequestEnemyTargetRefresh();
		return;
	}

	GetWorldTimerManager().ClearTimer(EliminationVisualTimerHandle);
	GetWorldTimerManager().ClearTimer(EliminationVFXStopTimerHandle);
	if (IsValid(ActiveEliminationVFXComponent))
	{
		ActiveEliminationVFXComponent->DeactivateImmediate();
		ActiveEliminationVFXComponent->DestroyComponent();
	}
	ActiveEliminationVFXComponent = nullptr;

	if (USkeletalMeshComponent* MainMesh = GetMesh())
	{
		MainMesh->SetVisibility(
			bMainMeshVisibleBeforeElimination,
			/*bPropagateToChildren=*/false);
	}
	if (IsValid(UEFNSourceMesh))
	{
		UEFNSourceMesh->SetVisibility(
			bSourceMeshVisibleBeforeElimination,
			/*bPropagateToChildren=*/false);
	}
	if (IsValid(PersistentWeaponMesh))
	{
		PersistentWeaponMesh->SetVisibility(bWeaponVisibleBeforeElimination);
	}

	SetActorEnableCollision(true);
	SetCanBeDamaged(true);
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->SetMovementMode(MOVE_Walking);
		MoveComp->StopMovementImmediately();
	}
	if (UGP_AbilitySystemComponent* GPASC = Cast<UGP_AbilitySystemComponent>(GetAbilitySystemComponent()))
	{
		GPASC->RefreshAutoActivatedAbilities();
	}
	RefreshCurrentMaxWalkSpeed();
	RequestEnemyTargetRefresh();
}

void AGP_PlayerCharacter::RefreshPartySpectatorTargetsOnServer() const
{
	UWorld* World = GetWorld();
	if (!HasAuthority() || !IsValid(World))
	{
		return;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator();
		It;
		++It)
	{
		if (AGP_PlayerController* PartyController =
			Cast<AGP_PlayerController>(It->Get()))
		{
			PartyController->RefreshEliminationSpectatingOnServer();
		}
	}
}

void AGP_PlayerCharacter::TryStopEliminationVFXEmission()
{
	if (!IsEliminated() || !IsValid(ActiveEliminationVFXComponent))
	{
		return;
	}

	++EliminationVFXEmissionFrameCount;
	const float ElapsedSeconds = GetWorld()
		? GetWorld()->GetTimeSeconds() - EliminationVFXStartTime
		: EliminationVFXEmissionTime;
	if (EliminationVFXEmissionFrameCount >= 3
		&& ElapsedSeconds >= FMath::Max(0.01f, EliminationVFXEmissionTime))
	{
		// Stop new grains while allowing emitted particles to complete their
		// scatter and auto-destroy. The frame gate survives a first-frame hitch.
		ActiveEliminationVFXComponent->Deactivate();
		return;
	}

	EliminationVFXStopTimerHandle =
		GetWorldTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(
				this,
				&ThisClass::TryStopEliminationVFXEmission));
}

void AGP_PlayerCharacter::TryRecoverFromElimination()
{
	if (!HasAuthority())
	{
		return;
	}

	AGP_PlayerState* GPPlayerState = GetPlayerState<AGP_PlayerState>();
	AGP_GameState* GPGameState = GetWorld() ? GetWorld()->GetGameState<AGP_GameState>() : nullptr;
	if (!IsValid(GPPlayerState) || !GPPlayerState->IsEliminated() || !IsValid(GPGameState))
	{
		return;
	}

	if (GPGameState->GetMatchPhase() == EGPMatchPhase::Victory
		|| GPGameState->GetMatchPhase() == EGPMatchPhase::Defeat)
	{
		return;
	}

	const auto ScheduleRecoveryRetry = [this]()
	{
		GetWorldTimerManager().SetTimer(
			EliminationRecoveryTimerHandle,
			this,
			&ThisClass::TryRecoverFromElimination,
			0.5f,
			false);
	};

	FTransform RecoveryAnchorTransform = FTransform::Identity;
	bool bHasRecoveryAnchor = false;
	if (AGP_GameMode* GPGameMode = GetWorld()->GetAuthGameMode<AGP_GameMode>())
	{
		bHasRecoveryAnchor =
			GPGameMode->TryGetIncompleteAssignedOuterRecoveryTransform(
				GPPlayerState,
				RecoveryAnchorTransform);
	}

	if (!bHasRecoveryAnchor)
	{
		if (AGP_PlayerCharacter* LivingRecoveryAnchor =
			FindLivingRecoveryAnchor())
		{
			RecoveryAnchorTransform =
				LivingRecoveryAnchor->GetActorTransform();
			bHasRecoveryAnchor = true;
		}
	}

	if (!bHasRecoveryAnchor)
	{
		// A living pawn can be briefly unavailable during possession/streaming.
		// Party-defeat evaluation will end the run when nobody is actually alive.
		ScheduleRecoveryRetry();
		return;
	}

	const float SideSign = (GPPlayerState->GetPlayerId() & 1) == 0 ? 1.0f : -1.0f;
	UWorld* World = GetWorld();
	UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (!IsValid(NavigationSystem) || !IsValid(Capsule))
	{
		ScheduleRecoveryRetry();
		return;
	}

	const FVector AnchorLocation = RecoveryAnchorTransform.GetLocation();
	const FVector AnchorForward =
		RecoveryAnchorTransform.GetUnitAxis(EAxis::X);
	const FVector AnchorRight =
		RecoveryAnchorTransform.GetUnitAxis(EAxis::Y);
	const TArray<FVector> RecoveryCandidates =
	{
		AnchorLocation - AnchorForward * 160.0f + AnchorRight * (260.0f * SideSign),
		AnchorLocation - AnchorForward * 160.0f - AnchorRight * (260.0f * SideSign),
		AnchorLocation - AnchorForward * 340.0f,
		AnchorLocation + AnchorRight * (340.0f * SideSign),
		AnchorLocation - AnchorRight * (340.0f * SideSign),
		AnchorLocation + AnchorForward * 300.0f
	};

	const float CapsuleRadius = Capsule->GetScaledCapsuleRadius();
	const float CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const FCollisionShape RecoveryCapsule = FCollisionShape::MakeCapsule(
		CapsuleRadius,
		CapsuleHalfHeight);
	FCollisionQueryParams RecoveryQueryParams(SCENE_QUERY_STAT(GP_EliminationRecovery), false, this);
	const FRotator RecoveryRotation = RecoveryAnchorTransform.Rotator();

	FVector SafeRecoveryLocation = FVector::ZeroVector;
	bool bFoundSafeRecoveryLocation = false;
	for (const FVector& RecoveryCandidate : RecoveryCandidates)
	{
		FNavLocation ProjectedRecoveryLocation;
		if (!NavigationSystem->ProjectPointToNavigation(
			RecoveryCandidate,
			ProjectedRecoveryLocation,
			FVector(300.0f, 300.0f, 500.0f)))
		{
			continue;
		}

		const FVector CandidateCapsuleCenter = ProjectedRecoveryLocation.Location
			+ FVector(0.0f, 0.0f, CapsuleHalfHeight + 2.0f);
		if (World->OverlapBlockingTestByProfile(
			CandidateCapsuleCenter,
			RecoveryRotation.Quaternion(),
			Capsule->GetCollisionProfileName(),
			RecoveryCapsule,
			RecoveryQueryParams))
		{
			continue;
		}

		SafeRecoveryLocation = CandidateCapsuleCenter;
		bFoundSafeRecoveryLocation = true;
		break;
	}

	if (!bFoundSafeRecoveryLocation)
	{
		ScheduleRecoveryRetry();
		return;
	}

	if (!TeleportTo(
		SafeRecoveryLocation,
		RecoveryRotation,
		/*bIsATest=*/false,
		// The capsule-profile overlap above is independent of this actor's
		// temporarily disabled collision, unlike TeleportTo's internal check.
		/*bNoCheck=*/true))
	{
		ScheduleRecoveryRetry();
		return;
	}

	if (UGP_AttributeSet* GPAttributeSet = Cast<UGP_AttributeSet>(GetAttributeSet()))
	{
		const float RecoveredHealth = FMath::Max(
			1.0f,
			GPAttributeSet->GetMaxHealth() * FMath::Clamp(RecoveryHealthFraction, 0.01f, 1.0f));
		GPAttributeSet->SetHealth(RecoveredHealth);
		GPAttributeSet->ResetOutOfHealthEventForRecovery();
	}

	GPPlayerState->SetEliminated(false);
	RefreshPartySpectatorTargetsOnServer();
}

AGP_PlayerCharacter* AGP_PlayerCharacter::FindLivingRecoveryAnchor() const
{
	const AGP_GameState* GPGameState = GetWorld() ? GetWorld()->GetGameState<AGP_GameState>() : nullptr;
	if (!IsValid(GPGameState))
	{
		return nullptr;
	}

	AGP_PlayerCharacter* BestAnchor = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();
	for (APlayerState* PartyPlayerState : GPGameState->PlayerArray)
	{
		const AGP_PlayerState* GPPartyPlayerState = Cast<AGP_PlayerState>(PartyPlayerState);
		AGP_PlayerCharacter* CandidatePawn = IsValid(GPPartyPlayerState)
			? Cast<AGP_PlayerCharacter>(GPPartyPlayerState->GetPawn())
			: nullptr;
		if (!IsValid(CandidatePawn)
			|| CandidatePawn == this
			|| GPPartyPlayerState->IsEliminated())
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(GetActorLocation(), CandidatePawn->GetActorLocation());
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestAnchor = CandidatePawn;
		}
	}

	return BestAnchor;
}

void AGP_PlayerCharacter::RequestEnemyTargetRefresh() const
{
	if (!HasAuthority() || !IsValid(GetWorld()))
	{
		return;
	}

	for (TActorIterator<AEnemyAIController> It(GetWorld()); It; ++It)
	{
		It->RequestTargetActorReevaluation();
	}
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
	else if (bPrimaryAttackInProgress)
	{
		return;
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
	if (!ASC || IsSprinting() || bPrimaryAttackInProgress) return;

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

void AGP_PlayerCharacter::SetPrimaryAttackInProgress(bool bInProgress)
{
	if (bPrimaryAttackInProgress == bInProgress)
	{
		return;
	}

	bPrimaryAttackInProgress = bInProgress;
	if (bPrimaryAttackInProgress)
	{
		StopSprinting();
	}
	else if (const AGP_PlayerController* GPController = Cast<AGP_PlayerController>(GetController()))
	{
		if (GPController->ShouldResumeHeldSprint())
		{
			StartSprinting();
		}
	}
	RefreshCurrentMaxWalkSpeed();
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
	const bool bEffectiveSprinting = bSprinting && !bPrimaryAttackInProgress;
	const float ForwardAmount = Input.Y;
	const float AbsForward = FMath::Abs(Input.Y);
	const float AbsRight = FMath::Abs(Input.X);

	float BaseSpeed = Profile.NormalForwardSpeed;
	if (FMath::IsNearlyZero(ForwardAmount) && AbsRight > 0.f)
	{
		BaseSpeed = bEffectiveSprinting ? Profile.SprintSideSpeed : Profile.NormalSideSpeed;
	}
	else if (ForwardAmount > 0.f)
	{
		BaseSpeed = bEffectiveSprinting ? Profile.SprintForwardSpeed : Profile.NormalForwardSpeed;
	}
	else if (AbsRight > AbsForward)
	{
		BaseSpeed = bEffectiveSprinting ? Profile.SprintSideSpeed : Profile.NormalSideSpeed;
	}
	else
	{
		BaseSpeed = bEffectiveSprinting ? Profile.SprintBackSpeed : Profile.NormalBackSpeed;
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
		MoveComp->MaxWalkSpeed = (IsSprinting() && !bPrimaryAttackInProgress) ? GetScaledSprintSpeed() : GetScaledNormalWalkSpeed();
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

	const float CharacterMeshScale = FMath::Max(VisualScaleProfile.CharacterMeshScale, 0.01f);
	float PartyVisualScale = 1.0f;
	if (PartyVisualSlot == 1)
	{
		PartyVisualScale = SecondPlayerVisualScale;
	}
	else if (PartyVisualSlot == 2)
	{
		PartyVisualScale = ThirdPlayerVisualScale;
	}
	GetMesh()->SetRelativeScale3D(FVector((CharacterMeshScale / UEFNSourceScale) * FMath::Max(PartyVisualScale, 0.01f)));
}

void AGP_PlayerCharacter::UpdateCameraMotion(float DeltaSeconds)
{
	if (!CameraBoom)
	{
		return;
	}

	const float Speed2D = GetVelocity().Size2D();
	const bool bShouldUseSprintCamera = IsSprinting() && !bPrimaryAttackInProgress;
	const bool bShouldUseIdleCamera = Speed2D <= IdleCameraSpeedThreshold && !bShouldUseSprintCamera;

	const float TargetArmLength = bShouldUseSprintCamera
		? SprintCameraArmLength
		: (bShouldUseIdleCamera ? IdleCameraArmLength : NormalCameraArmLength);

	const float ClampedArmInterpSpeed = FMath::Max(CameraArmLengthInterpSpeed, 0.0f);
	const float ClampedSocketInterpSpeed = FMath::Max(CameraSocketOffsetInterpSpeed, 0.0f);

	CameraBoom->TargetArmLength = ClampedArmInterpSpeed > 0.0f
		? FMath::FInterpTo(CameraBoom->TargetArmLength, TargetArmLength, DeltaSeconds, ClampedArmInterpSpeed)
		: TargetArmLength;

	CameraBoom->SocketOffset = ClampedSocketInterpSpeed > 0.0f
		? FMath::VInterpTo(CameraBoom->SocketOffset, CameraSocketOffset, DeltaSeconds, ClampedSocketInterpSpeed)
		: CameraSocketOffset;
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

void AGP_PlayerCharacter::OnRep_IsInWhiteVoid()
{
	PerformWhiteVoidTransition(bIsInWhiteVoid);
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
	const bool bHadPendingLagRestore = GetWorldTimerManager().IsTimerActive(RestoreLagTimerHandle);
	if (bHadPendingLagRestore)
	{
		GetWorldTimerManager().ClearTimer(RestoreLagTimerHandle);
	}

	if (bNewInWhiteVoid)
	{
		EnsureWhiteVoidSetExists();
	}

	CacheWhiteVoidTransitionState(!bHadPendingLagRestore);

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

void AGP_PlayerCharacter::CacheWhiteVoidTransitionState(bool bCacheCameraLag)
{
	if (CameraBoom && bCacheCameraLag)
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
		if (TrajectoryProperty->Struct == FTransformTrajectory::StaticStruct())
		{
			if (FTransformTrajectory* Trajectory = TrajectoryProperty->ContainerPtrToValuePtr<FTransformTrajectory>(TrajectoryComponent))
			{
				for (FTransformTrajectorySample& Sample : Trajectory->Samples)
				{
					Sample.Position = CurrentMeshLocation;
				}
			}
		}
	}

	if (FArrayProperty* HistoryProperty = FindFProperty<FArrayProperty>(TrajectoryComponent->GetClass(), TEXT("TranslationHistory")))
	{
		FScriptArrayHelper HistoryHelper(HistoryProperty, HistoryProperty->ContainerPtrToValuePtr<void>(TrajectoryComponent));
		if (FStructProperty* StructProperty = CastField<FStructProperty>(HistoryProperty->Inner))
		{
			UScriptStruct* InnerStruct = StructProperty->Struct;
			FStructProperty* PositionProperty = InnerStruct ? CastField<FStructProperty>(InnerStruct->FindPropertyByName(TEXT("Position"))) : nullptr;

			const UScriptStruct* VectorStruct = TBaseStructure<FVector>::Get();

			for (int32 Index = 0; Index < HistoryHelper.Num(); ++Index)
			{
				uint8* ValuePtr = HistoryHelper.GetRawPtr(Index);
				if (PositionProperty && PositionProperty->Struct == VectorStruct)
				{
					FVector* PosPtr = PositionProperty->ContainerPtrToValuePtr<FVector>(ValuePtr);
					if (PosPtr)
					{
						*PosPtr = CurrentMeshLocation;
					}
				}
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
	if (AGP_PlayerState* GPPlayerState = GetPlayerState<AGP_PlayerState>())
	{
		GPPlayerState->SetEquippedSkillData(SlotTag, NewSkillData);
	}
	// 교환 성공 알림 방송 (UI 연동용)
	if (OnSkillEquipped.IsBound())
	{
		OnSkillEquipped.Broadcast(SlotTag, NewSkillData);
	}
}

bool AGP_PlayerCharacter::ApplySkillLoadout(
	UGP_SkillData* Slot01SkillData,
	UGP_SkillData* Slot02SkillData)
{
	if (!HasAuthority())
	{
		return false;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	AGP_PlayerState* GPPlayerState = GetPlayerState<AGP_PlayerState>();
	if (!IsValid(ASC) || !IsValid(GPPlayerState))
	{
		return false;
	}

	auto IsValidSkillForSlot =
		[](const UGP_SkillData* SkillData, const FGameplayTag& SlotTag)
		{
			return !IsValid(SkillData)
				|| (SkillData->AbilityClass
					&& (SkillData->SupportedSlotTags.IsEmpty()
						|| SkillData->SupportedSlotTags.HasTagExact(SlotTag)));
		};
	if (!IsValidSkillForSlot(
			Slot01SkillData,
			GPTags::Ability::Skill::Slot01)
		|| !IsValidSkillForSlot(
			Slot02SkillData,
			GPTags::Ability::Skill::Slot02))
	{
		return false;
	}

	const auto AreSameSkill =
		[](const UGP_SkillData* Left, const UGP_SkillData* Right)
		{
			if (!IsValid(Left) || !IsValid(Right))
			{
				return false;
			}
			if (Left == Right)
			{
				return true;
			}
			if (Left->SkillIdTag.IsValid() && Right->SkillIdTag.IsValid())
			{
				return Left->SkillIdTag.MatchesTagExact(Right->SkillIdTag);
			}
			return Left->AbilityClass
				&& Left->AbilityClass == Right->AbilityClass;
		};
	if (AreSameSkill(Slot01SkillData, Slot02SkillData))
	{
		return false;
	}

	UGP_SkillData* CurrentSlot01Skill =
		GPPlayerState->GetEquippedSkillData(
			GPTags::Ability::Skill::Slot01);
	UGP_SkillData* CurrentSlot02Skill =
		GPPlayerState->GetEquippedSkillData(
			GPTags::Ability::Skill::Slot02);
	const bool bChangeSlot01 = CurrentSlot01Skill != Slot01SkillData;
	const bool bChangeSlot02 = CurrentSlot02Skill != Slot02SkillData;
	if (!bChangeSlot01 && !bChangeSlot02)
	{
		return true;
	}

	// Clear every changed slot before granting either replacement so swaps never
	// leave two active specs carrying the same skill identity.
	if (bChangeSlot01)
	{
		ClearAbilitySlot(GPTags::Ability::Skill::Slot01);
	}
	if (bChangeSlot02)
	{
		ClearAbilitySlot(GPTags::Ability::Skill::Slot02);
	}

	if (bChangeSlot01 && IsValid(Slot01SkillData))
	{
		if (!GiveAbilityToSlot(
				GPTags::Ability::Skill::Slot01,
				Slot01SkillData->AbilityClass,
				Slot01SkillData))
		{
			return false;
		}
	}
	if (bChangeSlot02 && IsValid(Slot02SkillData))
	{
		if (!GiveAbilityToSlot(
				GPTags::Ability::Skill::Slot02,
				Slot02SkillData->AbilityClass,
				Slot02SkillData))
		{
			return false;
		}
	}

	if (bChangeSlot01)
	{
		GPPlayerState->SetEquippedSkillData(
			GPTags::Ability::Skill::Slot01,
			Slot01SkillData);
		OnSkillEquipped.Broadcast(
			GPTags::Ability::Skill::Slot01,
			Slot01SkillData);
	}
	if (bChangeSlot02)
	{
		GPPlayerState->SetEquippedSkillData(
			GPTags::Ability::Skill::Slot02,
			Slot02SkillData);
		OnSkillEquipped.Broadcast(
			GPTags::Ability::Skill::Slot02,
			Slot02SkillData);
	}

	return true;
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
