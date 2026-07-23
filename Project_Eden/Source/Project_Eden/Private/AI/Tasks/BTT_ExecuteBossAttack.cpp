#include "AI/Tasks/BTT_ExecuteBossAttack.h"

#include "AI/Controllers/EnemyAIController.h"
#include "AI/Data/EnemyBlackboardKeys.h"
#include "AI/Tasks/BossAttackExecution.h"
#include "AI/Tasks/EnemyBTTaskCommon.h"
#include "BehaviorTree/BlackboardComponent.h"

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

	// The shared task owns smooth facing, target commitment, GAS tracking,
	// external pattern timing, and the recovery beat for every boss BT asset.
	return Super::ExecuteTask(OwnerComp, NodeMemory);
}
