#include "Characters/GP_EnemyCharacter.h"

#include "AI/Combat/EnemyAttackTransitionPolicy.h"
#include "AI/Controllers/EnemyAIController.h"
#include "AI/Data/EnemyBlackboardKeys.h"
#include "AIController.h"
#include "AI/Data/EnemyArchetypeData.h"
#include "AI/Data/EnemyLLMEvaluation.h"
#include "AI/Debug/EnemyAIDebugUtils.h"
#include "AI/Debug/EnemyAIRangeVisualizationComponent.h"
#include "BrainComponent.h"
#include "AbilitySystem/Abilities/Enemy/GP_BossAreaAttack.h"
#include "AbilitySystem/Abilities/Enemy/GP_BossBasicAttack.h"
#include "AbilitySystem/Abilities/Enemy/GP_BossGroundHandsAttack.h"
#include "AbilitySystem/Abilities/Enemy/GP_BossHeavyAttack.h"
#include "AbilitySystem/Abilities/Enemy/GP_BossSummonAdds.h"
#include "AbilitySystem/Abilities/Enemy/GP_BossSweepAttack.h"
#include "AbilitySystem/Abilities/Enemy/GP_EnemyAttack.h"
#include "AbilitySystem/Abilities/Enemy/GP_EnemyDeathAbility.h"
#include "AbilitySystem/GP_AbilitySystemComponent.h"
#include "AbilitySystem/GP_AttributeSet.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/PDA_EnemyAnimationSet.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/DataTable.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameplayTags/GP_Tags.h"
#include "Net/UnrealNetwork.h"
#include "Player/GP_PlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "UI/GP_AttributeWidget.h"
#include "UI/GP_WidgetComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UnrealType.h"
#include "VFX/GP_BossDeathPresentationComponent.h"
#include "VFX/GP_BossTargetMarkerVFXComponent.h"
#include "VFX/GP_EnemyDeathAbsorptionComponent.h"

AGP_EnemyCharacter::AGP_EnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// Blueprint-only regular enemies (including FurnaceWalker) otherwise inherit
	// CharacterMovement's unspecified turn defaults.  Keep their body aligned
	// with the chase path at a responsive, but not snapping, rate.  Enemy
	// subclasses can still override this in their own constructors.
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->bOrientRotationToMovement = true;
		MovementComponent->bUseControllerDesiredRotation = false;
		MovementComponent->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	}

	AbilitySystemComponent = CreateDefaultSubobject<UGP_AbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<UGP_AttributeSet>(TEXT("AttributeSet"));

	DefaultEnemyAttackAbilityClass = UGP_EnemyAttack::StaticClass();
	DefaultAttackAbilityTag = GPTags::Ability::Enemy::Attack_Melee;
	DefaultEnemyDeathAbilityClass = UGP_EnemyDeathAbility::StaticClass();

	BossTargetMarkerVFXComponent = CreateDefaultSubobject<UGP_BossTargetMarkerVFXComponent>(TEXT("BossTargetMarkerVFXComponent"));

	// Bosses use this dormant component to convert the shared GAS death state into a boss-specific clear effect.
	BossDeathPresentationComponent = CreateDefaultSubobject<UGP_BossDeathPresentationComponent>(TEXT("BossDeathPresentationComponent"));
	BossDeathPresentationComponent->SetupAttachment(GetRootComponent());

	// Regular enemies use a replicated cosmetic bridge; bosses keep their existing bespoke death presentation.
	EnemyDeathAbsorptionComponent = CreateDefaultSubobject<UGP_EnemyDeathAbsorptionComponent>(TEXT("EnemyDeathAbsorptionComponent"));

	WorldHealthBarComponent = CreateDefaultSubobject<UGP_WidgetComponent>(TEXT("WorldHealthBarComponent"));
	WorldHealthBarComponent->SetupAttachment(GetRootComponent());
	WorldHealthBarComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 135.0f));
	WorldHealthBarComponent->SetWidgetSpace(EWidgetSpace::Screen);
	WorldHealthBarComponent->SetDrawAtDesiredSize(true);
	WorldHealthBarComponent->SetPivot(FVector2D(0.5f, 1.0f));
	WorldHealthBarComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WorldHealthBarComponent->SetGenerateOverlapEvents(false);
	WorldHealthBarComponent->SetCastShadow(false);

	// Existing Blueprint enemies receive the shared health bar asset through their native parent automatically.
	static ConstructorHelpers::FClassFinder<UGP_AttributeWidget> EnemyHealthBarFinder(TEXT("/Game/UI/WBP_EnemyHealthBar"));
	if (EnemyHealthBarFinder.Succeeded())
	{
		WorldHealthBarComponent->SetWidgetClass(EnemyHealthBarFinder.Class);
	}

	// 적은 배치/스폰 시 공용 AIController를 자동 점유해 BT/Blackboard 초기화를 컨트롤러에 위임한다.
	AIControllerClass = AEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

#if WITH_EDITORONLY_DATA
	AIRangeVisualizer = CreateEditorOnlyDefaultSubobject<UEnemyAIRangeVisualizationComponent>(TEXT("AIRangeVisualizer"));
	if (AIRangeVisualizer != nullptr)
	{
		AIRangeVisualizer->SetupAttachment(GetRootComponent());
	}

	// Editor-only shapes make the gameplay ranges visible without adding runtime collision.
	RefreshAIRangeVisualizers();
#endif
}

void AGP_EnemyCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateTurnInPlace(DeltaSeconds);
	UpdateBasicEnemyCombatHoldFacing(DeltaSeconds);
}

UAttributeSet* AGP_EnemyCharacter::GetAttributeSet() const
{
	return AttributeSet;
}

void AGP_EnemyCharacter::UpdateAnimationSet()
{
	if (!IsValid(EnemyAnimationSet) && !DefaultEnemyAnimationSet.IsNull())
	{
		// Resolve native defaults only after actor components have initialized;
		// loading animation assets during CDO construction can re-enter PostLoad.
		EnemyAnimationSet = DefaultEnemyAnimationSet.LoadSynchronous();
	}

	if (!IsValid(EnemyAnimationSet))
	{
		// Existing enemies and bosses can continue using the legacy shared asset until migrated.
		Super::UpdateAnimationSet();
		return;
	}

	if (IsValid(EnemyAnimationSet->CharacterMesh))
	{
		GetMesh()->SetSkeletalMeshAsset(EnemyAnimationSet->CharacterMesh);
	}

	if (EnemyAnimationSet->AnimBlueprintClass)
	{
		GetMesh()->SetAnimInstanceClass(EnemyAnimationSet->AnimBlueprintClass);
	}
}

UAbilitySystemComponent* AGP_EnemyCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

FVector AGP_EnemyCharacter::GetBehaviorAnchorLocation() const
{
	// AI possession can ask for the anchor before BeginPlay, so compute the editor-authored point on demand.
	return bHasBehaviorAnchorLocation ? BehaviorAnchorLocation : GetActorTransform().TransformPosition(BehaviorAnchorOffset);
}

float AGP_EnemyCharacter::GetBasicMeleeAttackStartRange() const
{
	const float ForwardStepRange = FMath::Max(0.0f, BasicMeleeAttackStartRange);
	const float StationaryRange = FMath::Min(
		ForwardStepRange,
		FMath::Max(0.0f, BasicMeleeStationaryAttackStartRange));
	// Furnace-style lunges can use the wider edge; in-place melee such as production Cyclops stays inside overlap reach.
	return IsValid(EnemyAnimationSet) && EnemyAnimationSet->bUseAbilityForwardStep
		? ForwardStepRange
		: StationaryRange;
}

void AGP_EnemyCharacter::BeginBasicEnemyCombatHoldFacing(AActor* TargetActor, float TurnRateDegreesPerSecond)
{
	if (!HasAuthority() || bIsDead || !IsValid(TargetActor))
	{
		ClearBasicEnemyCombatHoldFacing();
		return;
	}

	BasicEnemyCombatHoldFacingTarget = TargetActor;
	BasicEnemyCombatHoldFacingTurnRateDegreesPerSecond = FMath::Max(0.0f, TurnRateDegreesPerSecond);
	// Tick is normally dormant for regular enemies; the explicit close-hold state temporarily enables per-frame facing.
	SetActorTickEnabled(true);
}

void AGP_EnemyCharacter::ClearBasicEnemyCombatHoldFacing()
{
	BasicEnemyCombatHoldFacingTarget.Reset();
	BasicEnemyCombatHoldFacingTurnRateDegreesPerSecond = 0.0f;
	// Preserve opt-in turn-in-place or derived persistent Tick after the temporary combat hold ends.
	SetActorTickEnabled(bEnableTurnInPlace || PrimaryActorTick.bStartWithTickEnabled);
}

void AGP_EnemyCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	RefreshWorldHealthBarVisibility();

	// Keep editor range rings in sync when designers move the anchor or change range values.
	RefreshAIRangeVisualizers();
}

bool AGP_EnemyCharacter::BuildInitialEnemyEvaluation(FEnemyLLMEvaluation& OutEvaluation) const
{
	if (const FEnemyArchetypeTuning* ArchetypeTuning = ResolveEnemyArchetypeTuning())
	{
		OutEvaluation = ArchetypeTuning->BuildEvaluation(ResolvePersonalitySeed());
		return true;
	}

	// 데이터가 없더라도 AI가 깨지지 않도록 안전한 기본값을 사용한다.
	OutEvaluation = FEnemyLLMEvaluation::MakeSafeDefault();
	return false;
}

FText AGP_EnemyCharacter::GetBossDisplayName() const
{
	if (!BossDisplayName.IsEmpty())
	{
		return BossDisplayName;
	}

	return FText::FromString(GetName());
}

void AGP_EnemyCharacter::NotifyBossTargetSelected(AActor* TargetActor)
{
	if (!bIsBossEnemy || bIsDead || !IsValid(BossTargetMarkerVFXComponent))
	{
		return;
	}

	// Keep the AIController free of Niagara details; boss pawns own how their selected target is presented.
	BossTargetMarkerVFXComponent->PlayTargetMarker(TargetActor);
}

void AGP_EnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		// Blueprint component defaults can override constructor values.  Reapply the
		// chase policy at runtime so walking enemies turn with their movement path,
		// rather than retaining a stale controller-facing rotation.
		MovementComponent->bOrientRotationToMovement = true;
		MovementComponent->bUseControllerDesiredRotation = false;
		MovementComponent->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	}
	RefreshWorldHealthBarVisibility();
	InitializeBasicEnemyAttackCadence();
	ApplyRuntimeMovementPolicy();

	if (IsValid(EnemyAnimationSet))
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			// Enemy attack movement is authored in lower-body montage root motion.
			AnimInstance->SetRootMotionMode(ERootMotionMode::RootMotionFromMontagesOnly);
		}
	}

	if (!IsValid(GetAbilitySystemComponent()))
	{
		return;
	}

	GetAbilitySystemComponent()->InitAbilityActorInfo(this, this);
	OnASCInitialized.Broadcast(GetAbilitySystemComponent(), GetAttributeSet());
	BindMoveSpeedAttribute();
	if (UGP_AttributeSet* EnemyAttributeSet = Cast<UGP_AttributeSet>(GetAttributeSet()))
	{
		// AttributeSet publishes the terminal health event; the enemy translates it into a GAS death request.
		EnemyAttributeSet->OnOutOfHealth.AddUniqueDynamic(this, &ThisClass::HandleOutOfHealth);
	}

	if (!HasAuthority())
	{
		return;
	}

	GiveStartupAbilities();
	GiveDefaultEnemyDeathAbility();
	GiveDefaultEnemyAttackAbility();
	if (bIsBossEnemy && bGrantDefaultBossPatternAbilities)
	{
		// Boss-specific defaults keep Sans prototypes playable even before designers add custom BP abilities.
		GiveDefaultBossPatternAbilities();
	}
	InitializeAttributes();
	const float AttributeMoveSpeed = GetAbilitySystemComponent()->GetNumericAttribute(UGP_AttributeSet::GetMoveSpeedAttribute());
	if (AttributeMoveSpeed > KINDA_SMALL_NUMBER)
	{
		if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
		{
			MovementComponent->MaxWalkSpeed = AttributeMoveSpeed;
		}
	}
	else if (const UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		GetAbilitySystemComponent()->SetNumericAttributeBase(
			UGP_AttributeSet::GetMoveSpeedAttribute(),
			MovementComponent->MaxWalkSpeed);
	}

	// 기준 위치는 캐릭터가 저장하고, 실제 Blackboard/Behavior Tree 시작은 AEnemyAIController::OnPossess에서 담당한다.
	BehaviorAnchorLocation = GetActorTransform().TransformPosition(BehaviorAnchorOffset);
	bHasBehaviorAnchorLocation = true;
}

void AGP_EnemyCharacter::ApplyRuntimeMovementPolicy()
{
	// Turn-in-place opts regular enemies into Tick, while derived actors can explicitly retain their own persistent Tick.
	SetActorTickEnabled(bEnableTurnInPlace || PrimaryActorTick.bStartWithTickEnabled);

	if (bIsBossEnemy)
	{
		if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
		{
			// Regular-enemy chase rotation must not override boss service/task-owned facing.
			MovementComponent->bOrientRotationToMovement = false;
		}
	}
}

void AGP_EnemyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Release the transient target reference before this replicated pawn leaves the world.
	ClearBasicEnemyCombatHoldFacing();
	StopTurnInPlace(false);
	StopActiveCombatTransitionMontage();
	CombatTransitionPhase = EGPEnemyCombatTransitionPhase::None;
	UnbindMoveSpeedAttribute();
	if (IsValid(BossTargetMarkerVFXComponent))
	{
		// Target marker VFX is attached to the player, so clear it explicitly when the boss leaves the world.
		BossTargetMarkerVFXComponent->HandleOwnerDeath();
	}

	Super::EndPlay(EndPlayReason);
}

void AGP_EnemyCharacter::UpdateTurnInPlace(float DeltaSeconds)
{
	if (!bEnableTurnInPlace || bIsDead)
	{
		return;
	}

	if (bTurnInPlaceActive)
	{
		if (bBasicEnemyAttackInProgress || IsBehaviorAttackCommitted())
		{
			// The BT commits during its facing phase, before GAS marks the attack in progress.
			StopTurnInPlace(true);
			return;
		}

		TurnInPlaceElapsedSeconds += DeltaSeconds;
		const float Alpha = FMath::Clamp(TurnInPlaceElapsedSeconds / FMath::Max(TurnInPlaceDurationSeconds, KINDA_SMALL_NUMBER), 0.0f, 1.0f);

		if (Alpha >= 1.0f)
		{
			StopTurnInPlace(false);
		}
		return;
	}

	// Do not infer a turn from Blackboard TargetActor here.  This Tick runs in every
	// stationary state, so that inference makes idle, recovery, and tactical waits
	// continuously face the player.  A dedicated AI task must call
	// StartTurnInPlaceForTarget when a turn is actually part of its state transition.
}

void AGP_EnemyCharacter::UpdateBasicEnemyCombatHoldFacing(float DeltaSeconds)
{
	AActor* TargetActor = BasicEnemyCombatHoldFacingTarget.Get();
	if (!HasAuthority() || bIsDead || bBasicEnemyAttackInProgress || !IsValid(TargetActor))
	{
		ClearBasicEnemyCombatHoldFacing();
		return;
	}

	const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (bTurnInPlaceActive || (IsValid(MovementComponent) && MovementComponent->Velocity.Size2D() > 5.0f))
	{
		return;
	}

	FVector LookDirection = TargetActor->GetActorLocation() - GetActorLocation();
	LookDirection.Z = 0.0f;
	if (LookDirection.IsNearlyZero())
	{
		return;
	}

	FRotator NewRotation = GetActorRotation();
	NewRotation.Yaw = EnemyAttackTransitionPolicy::StepFacingYaw(
		NewRotation.Yaw,
		LookDirection.Rotation().Yaw,
		BasicEnemyCombatHoldFacingTurnRateDegreesPerSecond,
		DeltaSeconds);
	// Server movement replication distributes this smooth close-hold yaw to all three connected players.
	SetActorRotation(NewRotation);
}

void AGP_EnemyCharacter::StartTurnInPlaceForTarget(const AActor* TargetActor)
{
	if (!IsValid(TargetActor))
	{
		return;
	}

	const FVector TargetLocation = TargetActor->GetActorLocation();
	TryStartTurnInPlace(&TargetLocation);
}

void AGP_EnemyCharacter::TryStartTurnInPlace(const FVector* OverrideTargetLocation)
{
	// Never start rotational root motion while the BT owns pre-attack facing or GAS owns the attack.
	if (bBasicEnemyAttackInProgress || IsBehaviorAttackCommitted() || !IsValid(GetController()))
	{
		return;
	}

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!IsValid(MovementComponent) || MovementComponent->Velocity.Size2D() > TurnInPlaceMaxStartSpeed)
	{
		return;
	}

	// Enemy controllers keep TargetActor in their Blackboard, but intentionally do not
	// drive ControlRotation from that target.  ControlRotation therefore remains aligned
	// to the pawn while stationary and cannot be used to detect an in-place turn.
	float DesiredYawDegrees = GetController()->GetControlRotation().Yaw;
	if (OverrideTargetLocation != nullptr)
	{
		const FVector TargetDirection = (*OverrideTargetLocation - GetActorLocation()).GetSafeNormal2D();
		if (!TargetDirection.IsNearlyZero())
		{
			DesiredYawDegrees = TargetDirection.Rotation().Yaw;
		}
	}
	else if (const AAIController* AIController = Cast<AAIController>(GetController()))
	{
		if (const UBlackboardComponent* BlackboardComponent = AIController->GetBlackboardComponent())
		{
			if (const AActor* TargetActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(EnemyBlackboardKeys::TargetActor)))
			{
				const FVector TargetDirection = (TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
				if (!TargetDirection.IsNearlyZero())
				{
					DesiredYawDegrees = TargetDirection.Rotation().Yaw;
				}
			}
		}
	}

	const float SignedYawDeltaDegrees = FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw, DesiredYawDegrees);
	if (FMath::Abs(SignedYawDeltaDegrees) < TurnInPlaceMinAngleDegrees)
	{
		return;
	}

	UAnimSequence* TurnSequence = SelectTurnInPlaceAnimation(SignedYawDeltaDegrees);
	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!IsValid(TurnSequence) || !IsValid(AnimInstance))
	{
		return;
	}

	const float SafePlayRate = FMath::Max(TurnInPlacePlayRate, 0.1f);
	// The retargeted turn sequences carry their own rotational root motion.
	// Let that montage turn the capsule so the body motion and facing stay synchronized.
	AnimInstance->SetRootMotionMode(ERootMotionMode::RootMotionFromMontagesOnly);
	if (!AnimInstance->PlaySlotAnimationAsDynamicMontage(TurnSequence, TurnInPlaceSlotName, 0.18f, 0.18f, SafePlayRate, 1, 0.0f, 0.0f))
	{
		return;
	}

	bTurnInPlaceActive = true;
	bRestoreOrientRotationToMovementAfterTurn = MovementComponent->bOrientRotationToMovement;
	UE_LOG(
		LogEnemyAI,
		Log,
		TEXT("[MoveTrace] TurnStop Pawn=%s Pos=%s TargetYaw=%.1f DeltaYaw=%.1f Seq=%s"),
		*GetName(),
		*GetActorLocation().ToCompactString(),
		DesiredYawDegrees,
		SignedYawDeltaDegrees,
		*GetNameSafe(TurnSequence));
	MovementComponent->StopMovementImmediately();
	MovementComponent->bOrientRotationToMovement = false;
	TurnInPlaceElapsedSeconds = 0.0f;
	TurnInPlaceDurationSeconds = TurnSequence->GetPlayLength() / SafePlayRate;
	SetTurnInPlaceAnimGraphFlag(true);
}

void AGP_EnemyCharacter::StopTurnInPlace(bool bStopAnimation)
{
	if (!bTurnInPlaceActive)
	{
		return;
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->bOrientRotationToMovement = bRestoreOrientRotationToMovementAfterTurn;
	}

	UE_LOG(
		LogEnemyAI,
		Log,
		TEXT("[MoveTrace] TurnEnd Pawn=%s Pos=%s StopAnimation=%d"),
		*GetName(),
		*GetActorLocation().ToCompactString(),
		bStopAnimation ? 1 : 0);

	if (bStopAnimation)
	{
		if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			AnimInstance->StopSlotAnimation(0.18f, TurnInPlaceSlotName);
		}
	}

	bTurnInPlaceActive = false;
	SetTurnInPlaceAnimGraphFlag(false);
}

float AGP_EnemyCharacter::BeginCombatTransitionAnimation(EGPEnemyCombatTransitionPhase TransitionPhase)
{
	if (!HasAuthority() || bIsDead || TransitionPhase == EGPEnemyCombatTransitionPhase::None)
	{
		return 0.0f;
	}

	const float TransitionDurationSeconds = GetCombatTransitionDurationSeconds(TransitionPhase);
	if (TransitionDurationSeconds <= KINDA_SMALL_NUMBER
		|| !IsValid(ResolveCombatTransitionAnimation(TransitionPhase)))
	{
		return 0.0f;
	}

	// Replicate only the semantic phase; every machine resolves the sequence from
	// the same enemy DataAsset and creates its cosmetic dynamic montage locally.
	CombatTransitionPhase = TransitionPhase;
	ApplyCombatTransitionAnimation();
	ForceNetUpdate();
	return TransitionDurationSeconds;
}

void AGP_EnemyCharacter::EndCombatTransitionAnimation()
{
	if (!HasAuthority())
	{
		return;
	}

	CombatTransitionPhase = EGPEnemyCombatTransitionPhase::None;
	ApplyCombatTransitionAnimation();
	ForceNetUpdate();
}

float AGP_EnemyCharacter::GetCombatTransitionDurationSeconds(EGPEnemyCombatTransitionPhase TransitionPhase) const
{
	if (!IsValid(EnemyAnimationSet) || !IsValid(ResolveCombatTransitionAnimation(TransitionPhase)))
	{
		return 0.0f;
	}

	switch (TransitionPhase)
	{
	case EGPEnemyCombatTransitionPhase::AttackPrepare:
		return FMath::Max(0.0f, EnemyAnimationSet->AttackPrepareDurationSeconds);
	case EGPEnemyCombatTransitionPhase::ChaseResume:
		return FMath::Max(0.0f, EnemyAnimationSet->ChaseResumeDurationSeconds);
	default:
		return 0.0f;
	}
}

UAnimSequence* AGP_EnemyCharacter::ResolveCombatTransitionAnimation(EGPEnemyCombatTransitionPhase TransitionPhase) const
{
	if (!IsValid(EnemyAnimationSet))
	{
		return nullptr;
	}

	switch (TransitionPhase)
	{
	case EGPEnemyCombatTransitionPhase::AttackPrepare:
		return EnemyAnimationSet->ResolveAttackPrepareAnimation();
	case EGPEnemyCombatTransitionPhase::ChaseResume:
		return EnemyAnimationSet->ResolveChaseResumeAnimation();
	default:
		return nullptr;
	}
}

void AGP_EnemyCharacter::ApplyCombatTransitionAnimation()
{
	StopActiveCombatTransitionMontage();
	if (bIsDead || CombatTransitionPhase == EGPEnemyCombatTransitionPhase::None)
	{
		return;
	}

	UAnimSequence* TransitionSequence = ResolveCombatTransitionAnimation(CombatTransitionPhase);
	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!IsValid(TransitionSequence) || !IsValid(AnimInstance) || !IsValid(EnemyAnimationSet))
	{
		return;
	}

	if (const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
		IsValid(ASC) && IsValid(ASC->GetCurrentMontage()))
	{
		// A late phase replication must never replace the GAS-owned attack montage.
		return;
	}

	ActiveCombatTransitionMontage = AnimInstance->PlaySlotAnimationAsDynamicMontage(
		TransitionSequence,
		EnemyAnimationSet->CombatTransitionSlotName,
		FMath::Max(0.0f, EnemyAnimationSet->CombatTransitionBlendInSeconds),
		FMath::Max(0.0f, EnemyAnimationSet->CombatTransitionBlendOutSeconds),
		1.0f,
		1,
		-1.0f,
		0.0f);

	UE_LOG(
		LogEnemyAI,
		Log,
		TEXT("[CombatTransition] Phase=%d Pawn=%s Animation=%s Duration=%.2f"),
		static_cast<int32>(CombatTransitionPhase),
		*GetNameSafe(this),
		*GetNameSafe(TransitionSequence),
		GetCombatTransitionDurationSeconds(CombatTransitionPhase));
}

void AGP_EnemyCharacter::StopActiveCombatTransitionMontage()
{
	UAnimMontage* TransitionMontage = ActiveCombatTransitionMontage.Get();
	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (IsValid(TransitionMontage) && IsValid(AnimInstance))
	{
		const float BlendOutSeconds = IsValid(EnemyAnimationSet)
			? FMath::Max(0.0f, EnemyAnimationSet->CombatTransitionBlendOutSeconds)
			: 0.12f;
		// Stop only the montage created for this bridge; never stop the slot broadly,
		// because a replicated GAS attack may already own DefaultSlot.
		AnimInstance->Montage_Stop(BlendOutSeconds, TransitionMontage);
	}
	ActiveCombatTransitionMontage = nullptr;
}

void AGP_EnemyCharacter::OnRep_CombatTransitionPhase()
{
	ApplyCombatTransitionAnimation();
}

UAnimSequence* AGP_EnemyCharacter::SelectTurnInPlaceAnimation(float SignedYawDeltaDegrees) const
{
	const bool bTurnLeft = SignedYawDeltaDegrees < 0.0f;
	const float AbsoluteYawDeltaDegrees = FMath::Abs(SignedYawDeltaDegrees);
	if (AbsoluteYawDeltaDegrees >= 157.5f)
	{
		return bTurnLeft ? TurnInPlaceAnimations.Turn180Left : TurnInPlaceAnimations.Turn180Right;
	}
	if (AbsoluteYawDeltaDegrees >= 112.5f)
	{
		return bTurnLeft ? TurnInPlaceAnimations.Turn135Left : TurnInPlaceAnimations.Turn135Right;
	}
	if (AbsoluteYawDeltaDegrees >= 67.5f)
	{
		return bTurnLeft ? TurnInPlaceAnimations.Turn90Left : TurnInPlaceAnimations.Turn90Right;
	}
	return bTurnLeft ? TurnInPlaceAnimations.Turn45Left : TurnInPlaceAnimations.Turn45Right;
}

void AGP_EnemyCharacter::SetTurnInPlaceAnimGraphFlag(bool bActive) const
{
	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!IsValid(AnimInstance))
	{
		return;
	}

	if (FBoolProperty* TurnFlagProperty = FindFProperty<FBoolProperty>(AnimInstance->GetClass(), TEXT("bTurnInPlaceActive")))
	{
		TurnFlagProperty->SetPropertyValue_InContainer(AnimInstance, bActive);
	}
}

bool AGP_EnemyCharacter::IsBasicEnemyAttackReady() const
{
	if (bIsBossEnemy)
	{
		// Boss pattern selectors own their cadence and must not inherit regular-enemy timing.
		return true;
	}

	const UWorld* World = GetWorld();
	return IsValid(World)
		&& !bBasicEnemyAttackInProgress
		&& !bTurnInPlaceActive
		&& CombatTransitionPhase == EGPEnemyCombatTransitionPhase::None
		&& EnemyAttackCadencePolicy::IsReady(World->GetTimeSeconds(), BasicEnemyAttackReadyTimeSeconds);
}

float AGP_EnemyCharacter::ScheduleNextBasicEnemyAttack()
{
	if (bIsBossEnemy || !HasAuthority())
	{
		return 0.0f;
	}

	const FVector2D DelayRange = EnemyAttackCadencePolicy::SanitizeDelayRange(
		AttackCadenceSettings.NextAttackDelayMinSeconds,
		AttackCadenceSettings.NextAttackDelayMaxSeconds);
	const float SelectedDelay = EnemyAttackCadencePolicy::RollDelay(DelayRange, AttackCadenceRandomStream);
	if (const UWorld* World = GetWorld())
	{
		// Only the authoritative enemy owns AI decisions, so this timestamp does not need replication.
		BasicEnemyAttackReadyTimeSeconds = World->GetTimeSeconds() + SelectedDelay;
	}
	return SelectedDelay;
}

bool AGP_EnemyCharacter::UpdateBehaviorAttackBandLatch(
	float DistanceToTarget,
	float MinAttackRange,
	float MaxAttackRange,
	bool bAllowAttacksInsidePreferredRange,
	float ExitHysteresis)
{
	bBehaviorAttackBandLatched = EnemyAttackTransitionPolicy::IsInsideAttackBand(
		DistanceToTarget,
		MinAttackRange,
		MaxAttackRange,
		bAllowAttacksInsidePreferredRange,
		bBehaviorAttackBandLatched,
		ExitHysteresis);
	return bBehaviorAttackBandLatched;
}

void AGP_EnemyCharacter::ResetBehaviorAttackBandLatch()
{
	// 타깃을 잃거나 귀환하면 다음 교전은 좁은 진입 범위에서 새로 시작한다.
	bBehaviorAttackBandLatched = false;
}

void AGP_EnemyCharacter::BeginBehaviorAttackCommit(AActor* TargetActor, float MaximumDurationSeconds)
{
	if (!HasAuthority() || bIsDead || !IsValid(TargetActor))
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	// 잘못된 몽타주/이벤트도 AI를 영구 정지시키지 않도록 태스크의 안전 타임아웃까지만 잠근다.
	BehaviorAttackCommittedTarget = TargetActor;
	BehaviorAttackCommitUntilTimeSeconds = World->GetTimeSeconds() + FMath::Max(0.0f, MaximumDurationSeconds);
}

void AGP_EnemyCharacter::FinishBehaviorAttackCommit(float RecoverySeconds)
{
	if (!HasAuthority())
	{
		return;
	}

	const UWorld* World = GetWorld();
	BehaviorAttackCommitUntilTimeSeconds = IsValid(World)
		? World->GetTimeSeconds() + FMath::Max(0.0f, RecoverySeconds)
		: 0.0f;
	if (RecoverySeconds <= KINDA_SMALL_NUMBER)
	{
		BehaviorAttackCommittedTarget.Reset();
	}
}

bool AGP_EnemyCharacter::IsBehaviorAttackCommitted() const
{
	const UWorld* World = GetWorld();
	// 타깃이 파괴되어 weak pointer가 먼저 비어도 이미 시작한 몽타주/타이머 액션의 잠금 수명은 유지한다.
	return !bIsDead
		&& IsValid(World)
		&& World->GetTimeSeconds() < BehaviorAttackCommitUntilTimeSeconds;
}

AActor* AGP_EnemyCharacter::GetBehaviorAttackCommittedTarget() const
{
	// 액션 commit과 타깃 생존 여부는 별도 계약이며, 파괴된 타깃은 호출자에게 노출하지 않는다.
	return IsBehaviorAttackCommitted() && BehaviorAttackCommittedTarget.IsValid()
		? BehaviorAttackCommittedTarget.Get()
		: nullptr;
}

void AGP_EnemyCharacter::InitializeBasicEnemyAttackCadence()
{
	if (bIsBossEnemy || !HasAuthority())
	{
		return;
	}

	// Actor names/IDs differ across spawned instances, preventing identical random streams in an encounter group.
	AttackCadenceRandomStream.Initialize(HashCombineFast(GetTypeHash(GetFName()), GetUniqueID()));
	const FVector2D InitialDelayRange = EnemyAttackCadencePolicy::SanitizeDelayRange(
		AttackCadenceSettings.InitialDelayMinSeconds,
		AttackCadenceSettings.InitialDelayMaxSeconds);
	const float InitialDelay = EnemyAttackCadencePolicy::RollDelay(InitialDelayRange, AttackCadenceRandomStream);
	if (const UWorld* World = GetWorld())
	{
		BasicEnemyAttackReadyTimeSeconds = World->GetTimeSeconds() + InitialDelay;
	}
}

void AGP_EnemyCharacter::BindMoveSpeedAttribute()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!IsValid(ASC) || MoveSpeedAttributeDelegateHandle.IsValid())
	{
		return;
	}

	MoveSpeedAttributeDelegateHandle = ASC
		->GetGameplayAttributeValueChangeDelegate(UGP_AttributeSet::GetMoveSpeedAttribute())
		.AddUObject(this, &ThisClass::HandleMoveSpeedAttributeChanged);
}

void AGP_EnemyCharacter::UnbindMoveSpeedAttribute()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!IsValid(ASC) || !MoveSpeedAttributeDelegateHandle.IsValid())
	{
		return;
	}

	ASC->GetGameplayAttributeValueChangeDelegate(UGP_AttributeSet::GetMoveSpeedAttribute())
		.Remove(MoveSpeedAttributeDelegateHandle);
	MoveSpeedAttributeDelegateHandle.Reset();
}

void AGP_EnemyCharacter::HandleMoveSpeedAttributeChanged(const FOnAttributeChangeData& ChangeData)
{
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->MaxWalkSpeed = FMath::Max(ChangeData.NewValue, 0.0f);
	}
}

const FEnemyArchetypeTuning* AGP_EnemyCharacter::ResolveEnemyArchetypeTuning() const
{
	if (IsValid(EnemyArchetypeData))
	{
		return &EnemyArchetypeData->Tuning;
	}

	if (EnemyArchetypeRow.DataTable != nullptr && EnemyArchetypeRow.RowName != NAME_None)
	{
		const FEnemyArchetypeTableRow* ArchetypeRow = EnemyArchetypeRow.GetRow<FEnemyArchetypeTableRow>(TEXT("AGP_EnemyCharacter::ResolveEnemyArchetypeTuning"));
		return ArchetypeRow != nullptr ? &ArchetypeRow->Tuning : nullptr;
	}

	if (bUseBuiltInArchetypeTuning)
	{
		return &BuiltInArchetypeTuning;
	}

	return nullptr;
}

int32 AGP_EnemyCharacter::ResolvePersonalitySeed() const
{
	if (bOverridePersonalitySeed)
	{
		return PersonalitySeedOverride;
	}

	// 같은 아키타입이라도 배치 위치와 이름이 다르면 미세한 개성 차이가 생기도록 시드를 만든다.
	return static_cast<int32>(
		HashCombineFast(
			GetTypeHash(GetFName()),
			HashCombineFast(GetTypeHash(GetActorLocation()), GetUniqueID())));
}

void AGP_EnemyCharacter::GiveDefaultBossPatternAbilities()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!IsValid(ASC))
	{
		return;
	}

	const TSubclassOf<UGameplayAbility> BossPatternAbilities[] =
	{
		UGP_BossBasicAttack::StaticClass(),
		UGP_BossHeavyAttack::StaticClass(),
		UGP_BossSweepAttack::StaticClass(),
		UGP_BossAreaAttack::StaticClass(),
		UGP_BossGroundHandsAttack::StaticClass(),
		UGP_BossSummonAdds::StaticClass(),
	};

	for (const TSubclassOf<UGameplayAbility>& AbilityClass : BossPatternAbilities)
	{
		if (!*AbilityClass || ASC->FindAbilitySpecFromClass(AbilityClass) != nullptr)
		{
			continue;
		}

		// Grant each default once; custom Blueprint abilities can still be added through StartupAbilities.
		ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass));
	}
}

void AGP_EnemyCharacter::GiveDefaultEnemyAttackAbility()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!bGrantDefaultEnemyAttackAbility || !IsValid(ASC) || !*DefaultEnemyAttackAbilityClass)
	{
		return;
	}

	if (DefaultAttackAbilityTag.IsValid())
	{
		for (const FGameplayAbilitySpec& AbilitySpec : ASC->GetActivatableAbilities())
		{
			if (IsValid(AbilitySpec.Ability)
				&& AbilitySpec.Ability->GetAssetTags().HasTagExact(DefaultAttackAbilityTag))
			{
				// A tag-compatible StartupAbility replaces the native fallback, preventing two specs from answering one BT attack.
				return;
			}
		}
	}

	// Class identity remains the fallback contract when an older enemy has no valid attack tag configured.
	if (ASC->FindAbilitySpecFromClass(DefaultEnemyAttackAbilityClass) == nullptr)
	{
		ASC->GiveAbility(FGameplayAbilitySpec(DefaultEnemyAttackAbilityClass));
	}
}

void AGP_EnemyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGP_EnemyCharacter, bIsDead);
	DOREPLIFETIME(AGP_EnemyCharacter, DeathInstigatorActor);
	DOREPLIFETIME(AGP_EnemyCharacter, CombatTransitionPhase);
}

void AGP_EnemyCharacter::GiveDefaultEnemyDeathAbility()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!bGrantDefaultEnemyDeathAbility || !IsValid(ASC) || !DefaultEnemyDeathAbilityClass)
	{
		return;
	}

	for (const FGameplayAbilitySpec& AbilitySpec : ASC->GetActivatableAbilities())
	{
		if (IsValid(AbilitySpec.Ability)
			&& AbilitySpec.Ability->GetAssetTags().HasTagExact(GPTags::Ability::Enemy::Death))
		{
			// A tag-compatible StartupAbility is the extension point for a future animated death ability.
			return;
		}
	}

	ASC->GiveAbility(FGameplayAbilitySpec(DefaultEnemyDeathAbilityClass));
}

void AGP_EnemyCharacter::HandlePostDamageTaken(AActor* InstigatorActor, float DamageAmount, FGameplayTag ElementTag)
{
	Super::HandlePostDamageTaken(InstigatorActor, DamageAmount, ElementTag);

	const UGP_AttributeSet* GPAttributeSet = Cast<UGP_AttributeSet>(GetAttributeSet());
	if (!HasAuthority() || bXPRewardGranted || !IsValid(GPAttributeSet) || GPAttributeSet->GetHealth() > KINDA_SMALL_NUMBER)
	{
		return;
	}

	bXPRewardGranted = true;
	GrantXPRewardToInstigator(InstigatorActor);

	// Same first-death gate as XP: let the GameMode count this enemy toward the current zone clear.
	OnEnemyDied.Broadcast(this);
}

void AGP_EnemyCharacter::HandleOutOfHealth(AActor* InstigatorActor, AActor* TargetActor)
{
	if (TargetActor != this || !HasAuthority())
	{
		return;
	}

	RequestDeath(InstigatorActor);
}

void AGP_EnemyCharacter::RequestDeath(AActor* InstigatorActor)
{
	if (!HasAuthority() || bIsDead || bDeathRequested)
	{
		return;
	}

	bDeathRequested = true;
	DeathInstigatorActor = InstigatorActor;
	UGP_AbilitySystemComponent* ASC = Cast<UGP_AbilitySystemComponent>(GetAbilitySystemComponent());
	const bool bDeathAbilityStarted = IsValid(ASC)
		&& ASC->TryActivateAbilityByTag(GPTags::Ability::Enemy::Death);
	if (!bDeathAbilityStarted)
	{
		// Death is a mandatory invariant, so a missing/misconfigured custom ability cannot leave a zero-health enemy alive.
		UE_LOG(LogTemp, Warning, TEXT("[EnemyDeath] Death ability failed; applying native fallback. Enemy=%s"), *GetNameSafe(this));
		EnterDeathStateFromAbility();
	}
}

void AGP_EnemyCharacter::EnterDeathStateFromAbility()
{
	if (!HasAuthority() || bIsDead)
	{
		return;
	}

	bIsDead = true;
	bDeathRequested = true;
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		// The persistent loose tag remains after the short death ability ends and identifies the terminal state to GAS queries.
		ASC->AddLooseGameplayTag(GPTags::Ability::Enemy::Death);
	}

	ApplyDeathState();
	ForceNetUpdate();
}

void AGP_EnemyCharacter::OnRep_IsDead()
{
	if (bIsDead)
	{
		ApplyDeathState();
	}
}

void AGP_EnemyCharacter::ApplyDeathState()
{
	if (bDeathStateApplied)
	{
		return;
	}

	bDeathStateApplied = true;
	if (HasAuthority())
	{
		EndCombatTransitionAnimation();
	}
	else
	{
		// OnRep_IsDead may arrive before the replicated phase reset.
		CombatTransitionPhase = EGPEnemyCombatTransitionPhase::None;
		ApplyCombatTransitionAnimation();
	}
	// 사망은 모든 행동 커밋보다 우선하며 지연된 BT 재평가가 타깃을 다시 잡지 못하게 한다.
	BehaviorAttackCommitUntilTimeSeconds = 0.0f;
	BehaviorAttackCommittedTarget.Reset();
	if (IsValid(BossTargetMarkerVFXComponent))
	{
		// Death must revoke selected-target presentation before delayed despawn can leave the boss corpse around.
		BossTargetMarkerVFXComponent->HandleOwnerDeath();
	}
	if (IsValid(BossDeathPresentationComponent))
	{
		// Presentation is local-only and no-ops for regular enemies, keeping the GAS death invariant centralised here.
		BossDeathPresentationComponent->PlayDeathPresentation(DeathInstigatorActor);
	}
	if (HasAuthority() && !bIsBossEnemy && IsValid(EnemyDeathAbsorptionComponent))
	{
		// Authority selects one absorption recipient, then the component multicasts that stable actor to every client.
		EnemyDeathAbsorptionComponent->PlayDeathAbsorption(DeathInstigatorActor);
	}

	RefreshWorldHealthBarVisibility();
	SetCanBeDamaged(false);
	SetActorTickEnabled(false);
	SetActorEnableCollision(false);

	if (UCapsuleComponent* EnemyCapsule = GetCapsuleComponent())
	{
		EnemyCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->DisableMovement();
	}

	// Notify presentation before scheduling destruction so a zero-delay setup can still react safely.
	OnEnemyDeathStarted.Broadcast(this, DeathInstigatorActor);
	BP_OnDeathStarted(DeathInstigatorActor);

	if (HasAuthority())
	{
		if (AAIController* AIController = Cast<AAIController>(GetController()))
		{
			AIController->StopMovement();
			if (UBrainComponent* BrainComponent = AIController->GetBrainComponent())
			{
				// Stop the Behavior Tree before detaching so no task can enqueue another attack during the corpse delay.
				BrainComponent->StopLogic(TEXT("Enemy health reached zero"));
			}
		}

		DetachFromControllerPendingDestroy();
		if (DeathDespawnDelay <= KINDA_SMALL_NUMBER)
		{
			Destroy();
		}
		else
		{
			SetLifeSpan(DeathDespawnDelay);
		}
	}
}

void AGP_EnemyCharacter::RefreshWorldHealthBarVisibility()
{
	if (!IsValid(WorldHealthBarComponent))
	{
		return;
	}

	// Bosses already have a dedicated HUD bar, while dead enemies must not leave an orphaned screen-space bar.
	WorldHealthBarComponent->SetVisibility(bShowWorldHealthBar && !bIsBossEnemy && !bIsDead, true);
}

void AGP_EnemyCharacter::GrantXPRewardToInstigator(AActor* InstigatorActor)
{
	if (XPReward <= 0.0f)
	{
		return;
	}

	// Shared-XP co-op: every player in the match gains the full reward, not just
	// the one who landed the killing blow. Server-authoritative (callers guard on
	// HasAuthority), so this loops the replicated PlayerArray once.
	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	if (!GameState)
	{
		// Fall back to the instigator alone if the game state isn't available yet.
		if (AGP_PlayerState* InstigatorPlayerState = ResolveInstigatorPlayerState(InstigatorActor))
		{
			InstigatorPlayerState->AddXP(XPReward);
		}
		return;
	}

	for (APlayerState* PartyPlayerState : GameState->PlayerArray)
	{
		if (AGP_PlayerState* GPPlayerState = Cast<AGP_PlayerState>(PartyPlayerState))
		{
			GPPlayerState->AddXP(XPReward);
		}
	}
}

AGP_PlayerState* AGP_EnemyCharacter::ResolveInstigatorPlayerState(AActor* InstigatorActor) const
{
	if (!IsValid(InstigatorActor))
	{
		return nullptr;
	}

	if (AGP_PlayerState* InstigatorPlayerState = Cast<AGP_PlayerState>(InstigatorActor))
	{
		return InstigatorPlayerState;
	}

	if (const APawn* InstigatorPawn = Cast<APawn>(InstigatorActor))
	{
		if (AController* InstigatorPawnController = InstigatorPawn->GetController())
		{
			return InstigatorPawnController->GetPlayerState<AGP_PlayerState>();
		}
	}

	if (const AController* InstigatorController = Cast<AController>(InstigatorActor))
	{
		return InstigatorController->GetPlayerState<AGP_PlayerState>();
	}

	if (AController* InstigatorController = InstigatorActor->GetInstigatorController())
	{
		return InstigatorController->GetPlayerState<AGP_PlayerState>();
	}

	return nullptr;
}

void AGP_EnemyCharacter::RefreshAIRangeVisualizers()
{
#if WITH_EDITORONLY_DATA
	if (AIRangeVisualizer != nullptr)
	{
		// The anchor offset is the same local point used by HomeLocation, so the rings preview runtime behavior.
		AIRangeVisualizer->SetRelativeLocation(BehaviorAnchorOffset);
		AIRangeVisualizer->ConfigureRanges(
			GetReturnHomeDistance(),
			GetPatrolRadius(),
			GetSightRadius(),
			GetLoseSightRadius(),
			GetPeripheralVisionAngleDegrees());
		AIRangeVisualizer->ConfigureVisibility(bShowAIRangesInEditor, bDrawAIRangesOnlyWhenSelected);
	}
#endif
}
