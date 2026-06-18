#pragma once

#include "AbilitySystem/Abilities/Enemy/GP_EnemyAttack.h"
#include "GP_EnemyRangedAttack.generated.h"

class AGP_EnemyRangedProjectile;

UCLASS()
class PROJECT_EDEN_API UGP_EnemyRangedAttack : public UGP_EnemyAttack
{
	GENERATED_BODY()

public:
	UGP_EnemyRangedAttack();

protected:
	virtual void PerformAttackHit() override;

	// Native fallback keeps BP_BasicEnemy_Ranged functional without an editor-authored projectile Blueprint.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Enemy|Ranged")
	TSubclassOf<AGP_EnemyRangedProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Enemy|Ranged", meta = (ClampMin = "0.0", Units = "cm"))
	float ProjectileSpawnForwardOffset = 110.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Enemy|Ranged", meta = (Units = "cm"))
	float ProjectileSpawnHeightOffset = 70.0f;
};
