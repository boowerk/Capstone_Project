#pragma once

#include "AI/Tasks/BTT_ExecuteEnemyAttack.h"
#include "BTT_ExecuteBossAttack.generated.h"

UCLASS()
class PROJECT_EDEN_API UBTT_ExecuteBossAttack : public UBTT_ExecuteEnemyAttack
{
	GENERATED_BODY()

public:
	UBTT_ExecuteBossAttack();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
