#include "AI/Tasks/BTT_ExecuteBossAttack.h"

#include "AbilitySystem/GP_AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AI/Controllers/EnemyAIController.h"
#include "AI/Data/EnemyBlackboardKeys.h"
#include "AI/Debug/EnemyAIDebugUtils.h"
#include "AI/Tasks/BossAttackExecution.h"
#include "AI/Tasks/EnemyBTTaskCommon.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameplayTags/GP_Tags.h"

UBTT_ExecuteBossAttack::UBTT_ExecuteBossAttack()
{
	// The BT owns the attack entry, while this node re-scores the pattern from live evaluation blackboard values every attack.
	NodeName = TEXT("Select And Execute Boss Attack");
	BossSweepAttackChance = 0.0f;
}

EBTNodeResult::Type UBTT_ExecuteBossAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = nullptr;
	APawn* ControlledPawn = nullptr;
	UBlackboardComponent* BlackboardComponent = nullptr;
	if (!EnemyBTTaskCommon::TryGetTaskContext(OwnerComp, AIController, ControlledPawn, BlackboardComponent))
	{
		return EBTNodeResult::Failed;
	}

	if (AEnemyAIController* EnemyAIController = Cast<AEnemyAIController>(AIController))
	{
		// Re-score the player target from the latest AI evaluation before choosing a boss pattern.
		EnemyAIController->RequestTargetActorReevaluation();
	}

	AActor* TargetActor = EnemyBTTaskCommon::GetTargetActor(BlackboardComponent);
	if (IsValid(TargetActor) && BossAttackExecution::HasBlackboardKey(BlackboardComponent, EnemyBlackboardKeys::DistanceToTarget))
	{
		BlackboardComponent->SetValueAsFloat(EnemyBlackboardKeys::DistanceToTarget, FVector::Dist2D(ControlledPawn->GetActorLocation(), TargetActor->GetActorLocation()));
	}

	if (bStopMovementBeforeAttack)
	{
		AIController->StopMovement();
	}

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
		UE_LOG(LogEnemyAI, Warning, TEXT("[BossAI] Cannot execute boss pattern - missing ASC: %s"), *GetNameSafe(ControlledPawn));
		return EBTNodeResult::Failed;
	}

	return BossAttackExecution::ExecuteBestPattern(ASC, ControlledPawn, BlackboardComponent, AttackAbilityTag, TargetActor);
}
