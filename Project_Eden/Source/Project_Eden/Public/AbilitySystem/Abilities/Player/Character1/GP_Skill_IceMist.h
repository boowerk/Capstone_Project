#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_TargetedSkillBase.h"
#include "GameplayTagContainer.h"
#include "GP_Skill_IceMist.generated.h"

class AGP_IceMistArea;
class UGameplayEffect;

UCLASS()
class PROJECT_EDEN_API UGP_Skill_IceMist : public UGP_TargetedSkillBase
{
	GENERATED_BODY()

public:
	UGP_Skill_IceMist();

protected:
	virtual void ExecuteConfirmedSkill_Implementation(const FGP_SkillTargetData& TargetData) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Ice Mist|Actor")
	TSubclassOf<AGP_IceMistArea> IceMistAreaClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Ice Mist|Effects")
	TSubclassOf<UGameplayEffect> SlowEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Ice Mist", meta = (ClampMin = "0.0"))
	float MistRadius = 300.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Ice Mist", meta = (ClampMin = "0.0", Units = "s"))
	float MistDuration = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Ice Mist", meta = (ClampMin = "0.01", Units = "s"))
	float DamageInterval = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Ice Mist|Movement", meta = (ClampMin = "0.0", Units = "cm/s"))
	float LaunchSpeed = 600.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Ice Mist|Movement", meta = (ClampMin = "0.0", Units = "s"))
	float MoveDuration = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Ice Mist|Movement", meta = (ClampMin = "0.0", Units = "cm"))
	float SpawnForwardOffset = 150.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Ice Mist|Movement", meta = (Units = "cm"))
	float SpawnUpOffset = 80.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Ice Mist")
	FGameplayTag HitEventTag;
};
