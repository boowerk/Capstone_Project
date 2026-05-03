#pragma once

#include "AI/Tasks/BTT_ExecuteEnemyAttack.h"
#include "CoreMinimal.h"
#include "BTT_BossAreaAttack.generated.h"

UCLASS()
class PROJECT_EDEN_API UBTT_BossAreaAttack : public UBTT_ExecuteEnemyAttack
{
	GENERATED_BODY()

public:
	UBTT_BossAreaAttack();
};
