#pragma once

#include "AI/Tasks/BTT_ExecuteEnemyAttack.h"
#include "CoreMinimal.h"
#include "BTT_BossPhaseShift.generated.h"

UCLASS()
class PROJECT_EDEN_API UBTT_BossPhaseShift : public UBTT_ExecuteEnemyAttack
{
	GENERATED_BODY()

public:
	UBTT_BossPhaseShift();
};
