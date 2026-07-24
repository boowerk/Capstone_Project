#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Enemy/GP_EnemyAttack.h"
#include "GP_BossAreaAttack.generated.h"

UCLASS(Blueprintable)
class PROJECT_EDEN_API UGP_BossAreaAttack : public UGP_EnemyAttack
{
	GENERATED_BODY()

public:
	UGP_BossAreaAttack();

protected:
	virtual UAnimMontage* ResolveConfiguredAttackMontage(AActor* AvatarActor) const override;
};
