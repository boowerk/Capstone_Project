#include "AI/Tasks/BTT_ExecuteEnemyAttack.h"

#include "AI/Combat/BossAttackTransitionPolicy.h"
#include "AI/Combat/EnemyAttackTransitionPolicy.h"
#include "AI/Tasks/EnemyBTTaskCommon.h"
#include "AbilitySystem/GP_AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "AI/Debug/EnemyAIDebugUtils.h"
#include "AI/Tasks/BossAttackExecution.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/GP_EnemyCharacter.h"
#include "Characters/GP_MatadorMageBossCharacter.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTags/GP_Tags.h"

namespace BTT_ExecuteEnemyAttack_Internal
{
	FGameplayTag ResolveAttackAbilityTagForContext(
		const APawn* ControlledPawn,
		const UBlackboardComponent* BlackboardComponent,
		const FGameplayTag& DefaultAttackAbilityTag,
		float BossSweepAttackChance)
	{
		(void)ControlledPawn;
		(void)BlackboardComponent;
		(void)BossSweepAttackChance;

		if (const AGP_EnemyCharacter* EnemyCharacter = Cast<AGP_EnemyCharacter>(ControlledPawn))
		{
			const FGameplayTag EnemyDefaultAttackTag = EnemyCharacter->GetDefaultAttackAbilityTag();
			if (EnemyDefaultAttackTag.IsValid()
				&& (!DefaultAttackAbilityTag.IsValid() || DefaultAttackAbilityTag.MatchesTagExact(GPTags::Ability::Enemy::Attack_Melee)))
			{
				// Shared enemy BT assets default to melee; enemy archetype parents can replace that with ranged or future tags.
				return EnemyDefaultAttackTag;
			}
		}

		// Generic attack tasks now execute their configured tag literally.
		// Boss pattern switching belongs to UBTT_ExecuteBossAttack or explicit boss task nodes in the BT.
		return DefaultAttackAbilityTag;
	}
}

UBTT_ExecuteEnemyAttack::UBTT_ExecuteEnemyAttack()
{
	NodeName = TEXT("Execute Enemy Attack");
	AttackAbilityTag = GPTags::Ability::Enemy::Attack_Melee;
	// 실행 상태는 AI마다 달라야 하므로 공유 템플릿 대신 BT 컴포넌트별 인스턴스를 사용한다.
	bCreateNodeInstance = true;
	bNotifyTick = true;
	bNotifyTaskFinished = true;
}

EBTNodeResult::Type UBTT_ExecuteEnemyAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ResetExecutionState();

	AAIController* AIController = nullptr;
	APawn* ControlledPawn = nullptr;
	UBlackboardComponent* BlackboardComponent = nullptr;
	if (!EnemyBTTaskCommon::TryGetTaskContext(OwnerComp, AIController, ControlledPawn, BlackboardComponent))
	{
		return EBTNodeResult::Failed;
	}

	const FGameplayTag EffectiveAttackAbilityTag = BTT_ExecuteEnemyAttack_Internal::ResolveAttackAbilityTagForContext(ControlledPawn, BlackboardComponent, AttackAbilityTag, BossSweepAttackChance);
	if (!EffectiveAttackAbilityTag.IsValid())
	{
		return EBTNodeResult::Failed;
	}

	AGP_EnemyCharacter* EnemyCharacter = Cast<AGP_EnemyCharacter>(ControlledPawn);
	bUseBossPatternSelector = BossAttackExecution::ShouldUseBossPatternSelector(ControlledPawn, BlackboardComponent);
	if (!bUseBossPatternSelector && IsValid(EnemyCharacter) && !EnemyCharacter->IsBasicEnemyAttackReady())
	{
		// 서비스와 태스크 사이의 짧은 레이스에서도 cadence를 우회한 중복 공격은 허용하지 않는다.
		return EBTNodeResult::Failed;
	}

	if (bStopMovementBeforeAttack)
	{
		AIController->StopMovement();
	}

	AActor* TargetActor = EnemyBTTaskCommon::GetTargetActor(BlackboardComponent);
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ControlledPawn);
	if (!IsValid(ASC))
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyAI] Cannot execute attack ability because ASC is missing: %s"), *GetNameSafe(ControlledPawn));
		return EBTNodeResult::Failed;
	}

	ActiveAbilitySystemComponent = ASC;
	ActiveEnemyCharacter = EnemyCharacter;
	ActiveControlledPawn = ControlledPawn;
	CommittedTargetActor = TargetActor;
	ActiveBlackboardComponent = BlackboardComponent;
	ActiveAbilityTag = EffectiveAttackAbilityTag;
	if (IsValid(EnemyCharacter))
	{
		EnemyCharacter->BeginBehaviorAttackCommit(TargetActor, AttackTimeoutSeconds);
	}

	if (bFaceTargetBeforeAttack && IsValid(TargetActor) && !IsFacingCommittedTarget())
	{
		// 이동 회전과 공격 조준이 동시에 ActorRotation을 덮어쓰지 않도록 준비 구간만 소유권을 가져온다.
		BeginFacingOwnership(ControlledPawn);
		ExecutionPhase = EEnemyAttackTaskPhase::FacingTarget;
		return EBTNodeResult::InProgress;
	}

	return ActivateBasicAttack()
		? EBTNodeResult::InProgress
		: EBTNodeResult::Failed;
}

bool UBTT_ExecuteEnemyAttack::ActivateBasicAttack()
{
	RestoreFacingOwnership();
	UAbilitySystemComponent* ASC = ActiveAbilitySystemComponent.Get();
	AGP_EnemyCharacter* EnemyCharacter = ActiveEnemyCharacter.Get();
	if (!IsValid(ASC) || !ActiveAbilityTag.IsValid())
	{
		if (IsValid(EnemyCharacter))
		{
			EnemyCharacter->FinishBehaviorAttackCommit(0.0f);
		}
		return false;
	}

	bool bActivated = false;
	if (bUseBossPatternSelector)
	{
		FGameplayTag ActivatedBossPatternTag;
		const EBTNodeResult::Type BossResult = BossAttackExecution::ExecuteBestPattern(
			ASC,
			ActiveControlledPawn.Get(),
			ActiveBlackboardComponent.Get(),
			ActiveAbilityTag,
			CommittedTargetActor.Get(),
			&ActivatedBossPatternTag);
		bActivated = BossResult == EBTNodeResult::Succeeded && ActivatedBossPatternTag.IsValid();
		if (bActivated)
		{
			// Track the concrete choice instead of the generic BT default tag.
			ActiveAbilityTag = ActivatedBossPatternTag;
			CurrentFallbackCommitSeconds = BossAttackTransitionPolicy::ResolvePostAbilityCommitSeconds(
				ActiveAbilityTag,
				BossDefaultPostAbilityCommitSeconds);
		}
	}
	else if (UGP_AbilitySystemComponent* GPASC = Cast<UGP_AbilitySystemComponent>(ASC))
	{
		bActivated = GPASC->TryActivateAbilityByTag(ActiveAbilityTag);
		CurrentFallbackCommitSeconds = InstantAttackCommitSeconds;
	}
	else
	{
		// Keep non-project ability system components compatible with the same gameplay tag contract.
		bActivated = ASC->TryActivateAbilitiesByTag(ActiveAbilityTag.GetSingleTagContainer());
		CurrentFallbackCommitSeconds = InstantAttackCommitSeconds;
	}

	if (bActivated)
	{
		bAttackActivationAccepted = true;
		ExecutionPhase = IsTrackedAbilityActive()
			? EEnemyAttackTaskPhase::AwaitingAbilityEnd
			: EEnemyAttackTaskPhase::FallbackCommit;
		PhaseElapsedSeconds = 0.0f;
		return true;
	}

	if (IsValid(EnemyCharacter))
	{
		EnemyCharacter->FinishBehaviorAttackCommit(0.0f);
	}
	return false;
}

void UBTT_ExecuteEnemyAttack::TickTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	PhaseElapsedSeconds += DeltaSeconds;
	TotalElapsedSeconds += DeltaSeconds;

	if (ExecutionPhase == EEnemyAttackTaskPhase::FacingTarget)
	{
		TickFacingTarget(OwnerComp, DeltaSeconds);
		return;
	}

	if (ExecutionPhase == EEnemyAttackTaskPhase::AwaitingAbilityEnd)
	{
		if (IsTrackedAbilityActive() && TotalElapsedSeconds < AttackTimeoutSeconds)
		{
			return;
		}

		if (TotalElapsedSeconds >= AttackTimeoutSeconds)
		{
			UE_LOG(
				LogEnemyAI,
				Warning,
				TEXT("[EnemyAI] Attack lifecycle timed out; releasing BT commit: Pawn=%s Tag=%s"),
				*EnemyAIDebugUtils::DescribeActor(ActiveEnemyCharacter.Get()),
				*ActiveAbilityTag.ToString());
		}

		if (TotalElapsedSeconds < AttackTimeoutSeconds
			&& bUseBossPatternSelector
			&& CurrentFallbackCommitSeconds > KINDA_SMALL_NUMBER)
		{
			// Some boss abilities finish GAS after the cast but leave an
			// authoritative projectile, beam, or delayed impact in progress.
			ExecutionPhase = EEnemyAttackTaskPhase::FallbackCommit;
			PhaseElapsedSeconds = 0.0f;
			return;
		}

		CompleteBasicAttack(OwnerComp);
		return;
	}

	if (ExecutionPhase == EEnemyAttackTaskPhase::FallbackCommit)
	{
		const bool bHasExternalActionSignal = ActiveAbilityTag.MatchesTagExact(
			GPTags::Ability::Enemy::Utility_MatadorBullPattern);
		const bool bExternalActionActive = IsTrackedExternalBossActionActive();
		if ((bHasExternalActionSignal && !bExternalActionActive)
			|| (!bHasExternalActionSignal && PhaseElapsedSeconds >= CurrentFallbackCommitSeconds)
			|| TotalElapsedSeconds >= AttackTimeoutSeconds)
		{
			CompleteBasicAttack(OwnerComp);
		}
		return;
	}

	const float EffectiveRecoverySeconds = bUseBossPatternSelector
		? BossAttackRecoverySeconds
		: AttackRecoverySeconds;
	if (ExecutionPhase == EEnemyAttackTaskPhase::Recovery
		&& PhaseElapsedSeconds >= EffectiveRecoverySeconds)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

EBTNodeResult::Type UBTT_ExecuteEnemyAttack::AbortTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	RestoreFacingOwnership();
	ScheduleBasicAttackCadence();
	if (AGP_EnemyCharacter* EnemyCharacter = ActiveEnemyCharacter.Get())
	{
		// 사망·귀환처럼 상위 전이가 태스크를 중단하면 회복 잠금도 즉시 해제한다.
		EnemyCharacter->FinishBehaviorAttackCommit(0.0f);
	}
	return EBTNodeResult::Aborted;
}

void UBTT_ExecuteEnemyAttack::OnTaskFinished(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	EBTNodeResult::Type TaskResult)
{
	if (TaskResult != EBTNodeResult::Succeeded)
	{
		ScheduleBasicAttackCadence();
		if (AGP_EnemyCharacter* EnemyCharacter = ActiveEnemyCharacter.Get())
		{
			EnemyCharacter->FinishBehaviorAttackCommit(0.0f);
		}
	}

	ResetExecutionState();
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

bool UBTT_ExecuteEnemyAttack::IsFacingCommittedTarget() const
{
	const APawn* ControlledPawn = ActiveControlledPawn.Get();
	const AActor* TargetActor = CommittedTargetActor.Get();
	if (!IsValid(ControlledPawn) || !IsValid(TargetActor))
	{
		return false;
	}

	FVector LookDirection = TargetActor->GetActorLocation() - ControlledPawn->GetActorLocation();
	LookDirection.Z = 0.0f;
	if (LookDirection.IsNearlyZero())
	{
		return true;
	}

	const float RemainingYaw = FMath::Abs(FMath::FindDeltaAngleDegrees(
		ControlledPawn->GetActorRotation().Yaw,
		LookDirection.Rotation().Yaw));
	return RemainingYaw <= FMath::Max(0.0f, AttackFacingToleranceDegrees);
}

void UBTT_ExecuteEnemyAttack::TickFacingTarget(
	UBehaviorTreeComponent& OwnerComp,
	float DeltaSeconds)
{
	APawn* ControlledPawn = ActiveControlledPawn.Get();
	const AActor* TargetActor = CommittedTargetActor.Get();
	if (!IsValid(ControlledPawn) || !IsValid(TargetActor))
	{
		if (AGP_EnemyCharacter* EnemyCharacter = ActiveEnemyCharacter.Get())
		{
			EnemyCharacter->FinishBehaviorAttackCommit(0.0f);
		}
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FVector LookDirection = TargetActor->GetActorLocation() - ControlledPawn->GetActorLocation();
	LookDirection.Z = 0.0f;
	if (!LookDirection.IsNearlyZero())
	{
		FRotator NewRotation = ControlledPawn->GetActorRotation();
		NewRotation.Yaw = EnemyAttackTransitionPolicy::StepFacingYaw(
			NewRotation.Yaw,
			LookDirection.Rotation().Yaw,
			AttackFacingTurnRateDegreesPerSecond,
			DeltaSeconds);
		// 제한된 SetActorRotation은 서버 권위 회전을 복제하면서 단일 프레임 스냅을 피한다.
		ControlledPawn->SetActorRotation(NewRotation);
	}

	if (IsFacingCommittedTarget() || PhaseElapsedSeconds >= MaximumAttackFacingSeconds)
	{
		if (!ActivateBasicAttack())
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		}
	}
}

void UBTT_ExecuteEnemyAttack::BeginFacingOwnership(APawn* ControlledPawn)
{
	ACharacter* Character = Cast<ACharacter>(ControlledPawn);
	UCharacterMovementComponent* MovementComponent = IsValid(Character)
		? Character->GetCharacterMovement()
		: nullptr;
	if (!IsValid(MovementComponent))
	{
		return;
	}

	FacingMovementComponent = MovementComponent;
	bPreviousOrientRotationToMovement = MovementComponent->bOrientRotationToMovement;
	bPreviousUseControllerDesiredRotation = MovementComponent->bUseControllerDesiredRotation;
	MovementComponent->bOrientRotationToMovement = false;
	MovementComponent->bUseControllerDesiredRotation = false;
	bOwnsFacingRotation = true;
}

void UBTT_ExecuteEnemyAttack::RestoreFacingOwnership()
{
	if (!bOwnsFacingRotation)
	{
		return;
	}

	if (UCharacterMovementComponent* MovementComponent = FacingMovementComponent.Get())
	{
		// 공격 조준이 끝나면 각 아키타입의 원래 이동 회전 설정을 그대로 돌려준다.
		MovementComponent->bOrientRotationToMovement = bPreviousOrientRotationToMovement;
		MovementComponent->bUseControllerDesiredRotation = bPreviousUseControllerDesiredRotation;
	}

	FacingMovementComponent.Reset();
	bOwnsFacingRotation = false;
}

bool UBTT_ExecuteEnemyAttack::IsTrackedAbilityActive() const
{
	const UAbilitySystemComponent* ASC = ActiveAbilitySystemComponent.Get();
	if (!IsValid(ASC) || !ActiveAbilityTag.IsValid())
	{
		return false;
	}

	for (const FGameplayAbilitySpec& AbilitySpec : ASC->GetActivatableAbilities())
	{
		const UGameplayAbility* Ability = AbilitySpec.Ability;
		if (AbilitySpec.IsActive()
			&& IsValid(Ability)
			&& Ability->GetAssetTags().HasTagExact(ActiveAbilityTag))
		{
			return true;
		}
	}

	return false;
}

bool UBTT_ExecuteEnemyAttack::IsTrackedExternalBossActionActive() const
{
	if (!ActiveAbilityTag.MatchesTagExact(GPTags::Ability::Enemy::Utility_MatadorBullPattern))
	{
		return false;
	}

	const AGP_MatadorMageBossCharacter* MatadorBoss = Cast<AGP_MatadorMageBossCharacter>(ActiveControlledPawn.Get());
	// Bull completion is observable on authoritative Matador state, so the task
	// releases as soon as the coordinator finishes instead of waiting a fixed cap.
	return IsValid(MatadorBoss) && MatadorBoss->IsBullPatternActive();
}

void UBTT_ExecuteEnemyAttack::CompleteBasicAttack(UBehaviorTreeComponent& OwnerComp)
{
	ScheduleBasicAttackCadence();
	const float EffectiveRecoverySeconds = bUseBossPatternSelector
		? BossAttackRecoverySeconds
		: AttackRecoverySeconds;
	if (AGP_EnemyCharacter* EnemyCharacter = ActiveEnemyCharacter.Get())
	{
		// cadence는 공격 시작이 아니라 실제 종료 뒤부터 계산해 긴 몽타주와 다음 공격이 겹치지 않게 한다.
		EnemyCharacter->FinishBehaviorAttackCommit(EffectiveRecoverySeconds);
	}

	ExecutionPhase = EEnemyAttackTaskPhase::Recovery;
	PhaseElapsedSeconds = 0.0f;
	if (EffectiveRecoverySeconds <= KINDA_SMALL_NUMBER)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

void UBTT_ExecuteEnemyAttack::ScheduleBasicAttackCadence()
{
	AGP_EnemyCharacter* EnemyCharacter = ActiveEnemyCharacter.Get();
	if (bCadenceScheduled
		|| !bAttackActivationAccepted
		|| !IsValid(EnemyCharacter)
		|| EnemyCharacter->IsBossEnemy())
	{
		return;
	}

	bCadenceScheduled = true;
	const float NextAttackDelay = EnemyCharacter->ScheduleNextBasicEnemyAttack();
	UE_LOG(
		LogEnemyAI,
		Verbose,
		TEXT("[AttackCadence] Recovery complete; next attack in %.2fs: Pawn=%s"),
		NextAttackDelay,
		*EnemyAIDebugUtils::DescribeActor(EnemyCharacter));
}

void UBTT_ExecuteEnemyAttack::ResetExecutionState()
{
	RestoreFacingOwnership();
	ExecutionPhase = EEnemyAttackTaskPhase::Idle;
	ActiveAbilitySystemComponent.Reset();
	ActiveEnemyCharacter.Reset();
	ActiveControlledPawn.Reset();
	CommittedTargetActor.Reset();
	ActiveBlackboardComponent.Reset();
	ActiveAbilityTag = FGameplayTag();
	PhaseElapsedSeconds = 0.0f;
	TotalElapsedSeconds = 0.0f;
	CurrentFallbackCommitSeconds = 0.0f;
	bAttackActivationAccepted = false;
	bCadenceScheduled = false;
	bUseBossPatternSelector = false;
	bPreviousOrientRotationToMovement = false;
	bPreviousUseControllerDesiredRotation = false;
}

FString UBTT_ExecuteEnemyAttack::GetStaticDescription() const
{
	return FString::Printf(
		TEXT("Attack tag: %s\nWait for GAS/external action, recovery %.2fs (boss %.2fs)"),
		*AttackAbilityTag.ToString(),
		AttackRecoverySeconds,
		BossAttackRecoverySeconds);
}
