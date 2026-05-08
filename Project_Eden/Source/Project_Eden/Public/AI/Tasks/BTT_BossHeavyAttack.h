#pragma once

#include "AI/Tasks/BTT_ExecuteEnemyAttack.h"
#include "CoreMinimal.h"
#include "BTT_BossHeavyAttack.generated.h"

UCLASS()
class PROJECT_EDEN_API UBTT_BossHeavyAttack : public UBTT_ExecuteEnemyAttack
{
	GENERATED_BODY()

public:
	UBTT_BossHeavyAttack();
};
