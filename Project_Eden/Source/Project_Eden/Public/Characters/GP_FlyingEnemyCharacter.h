#pragma once

#include "Characters/GP_EnemyCharacter.h"
#include "GP_FlyingEnemyCharacter.generated.h"

UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_FlyingEnemyCharacter : public AGP_EnemyCharacter
{
	GENERATED_BODY()

public:
	AGP_FlyingEnemyCharacter();
	virtual void UpdateAnimationSet() override;

protected:
	virtual void BeginPlay() override;
};
