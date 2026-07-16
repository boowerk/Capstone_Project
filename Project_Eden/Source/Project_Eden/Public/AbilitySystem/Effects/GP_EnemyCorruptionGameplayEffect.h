#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GP_EnemyCorruptionGameplayEffect.generated.h"

/** Native infinite effect whose SetByCaller modifiers carry the current regional corruption strength. */
UCLASS()
class PROJECT_EDEN_API UGP_EnemyCorruptionGameplayEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGP_EnemyCorruptionGameplayEffect();
};
