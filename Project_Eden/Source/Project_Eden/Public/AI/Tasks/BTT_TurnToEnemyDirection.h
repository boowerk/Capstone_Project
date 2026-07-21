#pragma once

#include "BehaviorTree/BTTaskNode.h"
#include "BTT_TurnToEnemyDirection.generated.h"

/** Waits for an authored in-place turn before the following movement or attack node runs. */
UCLASS()
class PROJECT_EDEN_API UBTT_TurnToEnemyDirection : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTT_TurnToEnemyDirection();

	/** Move branches use MoveToLocation; attack branches use TargetActor. */
	UPROPERTY(EditAnywhere, Category = "AI|Turn")
	bool bUseMoveToLocation = false;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
