#include "AI/Tasks/BTT_ExecuteEnemyAttack.h"

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
	const bool bUseBossPatternSelector = BossAttackExecution::ShouldUseBossPatternSelector(ControlledPawn, BlackboardComponent);
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
	if (bFaceTargetBeforeAttack && IsValid(TargetActor))
	{
		FVector LookDirection = TargetActor->GetActorLocation() - ControlledPawn->GetActorLocation();
		LookDirection.Z = 0.0f;
		if (!LookDirection.IsNearlyZero())
		{
			ControlledPawn->SetActorRotation(LookDirection.Rotation());
		}
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ControlledPawn);
	if (!IsValid(ASC))
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyAI] Cannot execute attack ability because ASC is missing: %s"), *GetNameSafe(ControlledPawn));
		return EBTNodeResult::Failed;
	}

	if (bUseBossPatternSelector)
	{
		// Some merged boss BT assets still reference this generic task; route boss pawns through the shared pattern selector.
		UE_LOG(
			LogEnemyAI,
			Log,
			TEXT("[BossAI] Generic attack task routed through boss pattern selector: Pawn=%s Target=%s"),
			*GetNameSafe(ControlledPawn),
			*EnemyAIDebugUtils::DescribeActor(TargetActor));
		return BossAttackExecution::ExecuteBestPattern(ASC, ControlledPawn, BlackboardComponent, EffectiveAttackAbilityTag, TargetActor);
	}

	ActiveAbilitySystemComponent = ASC;
	ActiveEnemyCharacter = EnemyCharacter;
	CommittedTargetActor = TargetActor;
	ActiveAbilityTag = EffectiveAttackAbilityTag;
	if (IsValid(EnemyCharacter))
	{
		EnemyCharacter->BeginBehaviorAttackCommit(TargetActor, AttackTimeoutSeconds);
	}

	bool bActivated = false;

	if (UGP_AbilitySystemComponent* GPASC = Cast<UGP_AbilitySystemComponent>(ASC))
	{
		bActivated = GPASC->TryActivateAbilityByTag(EffectiveAttackAbilityTag);
	}
	else
	{
		// Keep non-project ability system components compatible with the same gameplay tag contract.
		bActivated = ASC->TryActivateAbilitiesByTag(EffectiveAttackAbilityTag.GetSingleTagContainer());
	}

	if (bActivated)
	{
		bAttackActivationAccepted = true;
		ExecutionPhase = IsTrackedAbilityActive()
			? EEnemyAttackTaskPhase::AwaitingAbilityEnd
			: EEnemyAttackTaskPhase::FallbackCommit;
		return EBTNodeResult::InProgress;
	}

	if (IsValid(EnemyCharacter))
	{
		EnemyCharacter->FinishBehaviorAttackCommit(0.0f);
	}
	return EBTNodeResult::Failed;
}

void UBTT_ExecuteEnemyAttack::TickTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	PhaseElapsedSeconds += DeltaSeconds;
	TotalElapsedSeconds += DeltaSeconds;

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
		CompleteBasicAttack(OwnerComp);
		return;
	}

	if (ExecutionPhase == EEnemyAttackTaskPhase::FallbackCommit)
	{
		if (PhaseElapsedSeconds >= InstantAttackCommitSeconds)
		{
			CompleteBasicAttack(OwnerComp);
		}
		return;
	}

	if (ExecutionPhase == EEnemyAttackTaskPhase::Recovery
		&& PhaseElapsedSeconds >= AttackRecoverySeconds)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

EBTNodeResult::Type UBTT_ExecuteEnemyAttack::AbortTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
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

void UBTT_ExecuteEnemyAttack::CompleteBasicAttack(UBehaviorTreeComponent& OwnerComp)
{
	ScheduleBasicAttackCadence();
	if (AGP_EnemyCharacter* EnemyCharacter = ActiveEnemyCharacter.Get())
	{
		// cadence는 공격 시작이 아니라 실제 종료 뒤부터 계산해 긴 몽타주와 다음 공격이 겹치지 않게 한다.
		EnemyCharacter->FinishBehaviorAttackCommit(AttackRecoverySeconds);
	}

	ExecutionPhase = EEnemyAttackTaskPhase::Recovery;
	PhaseElapsedSeconds = 0.0f;
	if (AttackRecoverySeconds <= KINDA_SMALL_NUMBER)
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
	ExecutionPhase = EEnemyAttackTaskPhase::Idle;
	ActiveAbilitySystemComponent.Reset();
	ActiveEnemyCharacter.Reset();
	CommittedTargetActor.Reset();
	ActiveAbilityTag = FGameplayTag();
	PhaseElapsedSeconds = 0.0f;
	TotalElapsedSeconds = 0.0f;
	bAttackActivationAccepted = false;
	bCadenceScheduled = false;
}

FString UBTT_ExecuteEnemyAttack::GetStaticDescription() const
{
	return FString::Printf(
		TEXT("Attack tag: %s\nWait for GAS end, recovery %.2fs"),
		*AttackAbilityTag.ToString(),
		AttackRecoverySeconds);
}
