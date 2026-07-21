#pragma once

#include "Characters/GP_EnemyCharacter.h"
#include "GP_RangedEnemyCharacter.generated.h"

UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_RangedEnemyCharacter : public AGP_EnemyCharacter
{
	GENERATED_BODY()

public:
	AGP_RangedEnemyCharacter();
	virtual void UpdateAnimationSet() override;
};
