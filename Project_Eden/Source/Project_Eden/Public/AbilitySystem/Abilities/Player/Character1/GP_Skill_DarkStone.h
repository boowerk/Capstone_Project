#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_TargetedSkillBase.h"
#include "GP_Skill_DarkStone.generated.h"

class AGP_GroundLaunchProjectile;

UCLASS()
class PROJECT_EDEN_API UGP_Skill_DarkStone : public UGP_TargetedSkillBase
{
	GENERATED_BODY()

public:
	UGP_Skill_DarkStone();

protected:
	virtual void ExecuteConfirmedSkill_Implementation(const FGP_SkillTargetData& TargetData) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Dark Stone")
	TSubclassOf<AGP_GroundLaunchProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Dark Stone", meta = (ClampMin = "0.0"))
	float GroundForwardOffset = 180.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Dark Stone", meta = (ClampMin = "0.0"))
	float RiseDepth = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Dark Stone", meta = (ClampMin = "0.0"))
	float RiseHeight = 120.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Dark Stone", meta = (ClampMin = "0.01", Units = "s"))
	float RiseDuration = 0.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Dark Stone", meta = (ClampMin = "0.0", Units = "s"))
	float LaunchDelay = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Dark Stone", meta = (ClampMin = "0.0"))
	float LaunchSpeed = 900.f;
};
