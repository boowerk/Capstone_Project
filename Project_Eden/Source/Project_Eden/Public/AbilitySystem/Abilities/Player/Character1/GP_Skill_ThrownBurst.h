#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_SkillBase.h"
#include "GP_Skill_ThrownBurst.generated.h"

class AGP_AreaProjectile;

/**
 * Throws an area projectile that explodes on impact.
 */
UCLASS()
class PROJECT_EDEN_API UGP_Skill_ThrownBurst : public UGP_SkillBase
{
	GENERATED_BODY()

public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|ThrownBurst")
	TSubclassOf<AGP_AreaProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|ThrownBurst")
	float SpawnForwardOffset = 120.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|ThrownBurst")
	float SpawnUpOffset = 70.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|ThrownBurst")
	float MinPitch = -10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|ThrownBurst")
	float MaxPitch = 35.f;
};
