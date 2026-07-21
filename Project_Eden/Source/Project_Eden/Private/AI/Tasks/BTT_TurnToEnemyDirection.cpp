#include "AI/Tasks/BTT_TurnToEnemyDirection.h"

#include "AI/Data/EnemyBlackboardKeys.h"
#include "AI/Tasks/EnemyBTTaskCommon.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/GP_EnemyCharacter.h"

UBTT_TurnToEnemyDirection::UBTT_TurnToEnemyDirection()
{
	NodeName = TEXT("Turn To Enemy Direction");
	bNotifyTick = true;
}

EBTNodeResult::Type UBTT_TurnToEnemyDirection::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = nullptr;
	APawn* ControlledPawn = nullptr;
	UBlackboardComponent* BlackboardComponent = nullptr;
	if (!EnemyBTTaskCommon::TryGetTaskContext(OwnerComp, AIController, ControlledPawn, BlackboardComponent))
	{
		return EBTNodeResult::Failed;
	}

	AGP_EnemyCharacter* EnemyCharacter = Cast<AGP_EnemyCharacter>(ControlledPawn);
	if (!IsValid(EnemyCharacter))
	{
		return EBTNodeResult::Succeeded;
	}

	if (EnemyCharacter->IsTurnInPlaceActive())
	{
		return EBTNodeResult::InProgress;
	}

	AActor* TargetActor = EnemyBTTaskCommon::GetTargetActor(BlackboardComponent);
	const bool bIsPositionalMove = bUseMoveToLocation
		&& (!IsValid(TargetActor)
			|| BlackboardComponent->GetValueAsBool(EnemyBlackboardKeys::bShouldRetreat)
			|| BlackboardComponent->GetValueAsBool(EnemyBlackboardKeys::bShouldReposition)
			|| BlackboardComponent->GetValueAsBool(EnemyBlackboardKeys::bShouldReturnHome));

	// Chase MoveTo nodes may still carry an old MoveToLocation from a previous
	// reposition.  Their intentional direction is TargetActor, never that stale vector.
	const bool bStartedTurn = bIsPositionalMove
		? EnemyCharacter->StartTurnInPlaceForLocation(BlackboardComponent->GetValueAsVector(EnemyBlackboardKeys::MoveToLocation))
		: EnemyCharacter->StartTurnInPlaceForTarget(TargetActor);
	return bStartedTurn ? EBTNodeResult::InProgress : EBTNodeResult::Succeeded;
}

void UBTT_TurnToEnemyDirection::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AIController = nullptr;
	APawn* ControlledPawn = nullptr;
	UBlackboardComponent* BlackboardComponent = nullptr;
	if (!EnemyBTTaskCommon::TryGetTaskContext(OwnerComp, AIController, ControlledPawn, BlackboardComponent))
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	const AGP_EnemyCharacter* EnemyCharacter = Cast<AGP_EnemyCharacter>(ControlledPawn);
	if (!IsValid(EnemyCharacter) || !EnemyCharacter->IsTurnInPlaceActive())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}
