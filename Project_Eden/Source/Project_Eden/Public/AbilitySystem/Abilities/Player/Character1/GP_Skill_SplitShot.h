#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_SkillBase.h"
#include "GP_Skill_SplitShot.generated.h"

class AGP_Projectile;

/**
 * Fires multiple projectiles in a horizontal fan.
 */
UCLASS()
class PROJECT_EDEN_API UGP_Skill_SplitShot : public UGP_SkillBase
{
	GENERATED_BODY()

public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Projectile")
	TSubclassOf<AGP_Projectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Projectile", meta = (ClampMin = "1"))
	int32 ProjectileCount = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Projectile", meta = (ClampMin = "0.0"))
	float TotalSpreadAngle = 20.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Projectile")
	float SpawnForwardOffset = 120.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Projectile")
	float SpawnUpOffset = 60.f;
};
