#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Enemy/GP_EnemyAttack.h"
#include "GP_BossBasicAttack.generated.h"

UCLASS(Blueprintable)
class PROJECT_EDEN_API UGP_BossBasicAttack : public UGP_EnemyAttack
{
	GENERATED_BODY()

public:
	UGP_BossBasicAttack();
};
