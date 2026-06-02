#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_TargetedSkillBase.h"
#include "GP_Skill_LifeDrainTarget.generated.h"

class UAbilitySystemComponent;

/**
 * Target-select channeled drain sample.
 * Selects a target near the aim line, then drains while range and line-of-sight remain valid.
 */
UCLASS()
class PROJECT_EDEN_API UGP_Skill_LifeDrainTarget : public UGP_TargetedSkillBase
{
	GENERATED_BODY()

public:
	UGP_Skill_LifeDrainTarget();

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	virtual void ExecuteConfirmedSkill_Implementation(const FGP_SkillTargetData& TargetData) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Drain", meta = (ClampMin = "0.1"))
	float DrainDuration = 4.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Drain", meta = (ClampMin = "0.05"))
	float DrainTickInterval = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Drain", meta = (ClampMin = "0.0"))
	float DrainDamagePerTick = 8.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Drain", meta = (ClampMin = "0.0"))
	float HealRatio = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Drain")
	TSubclassOf<UGameplayEffect> HealEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Drain", meta = (ClampMin = "1.0"))
	float MaintainRangeMultiplier = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Drain")
	bool bDrawDrainDebug = false;

private:
	void TickDrain();
	bool CanContinueDrain() const;
	void ApplyDrainTick();
	void FinishDrain(bool bWasCancelled);

	UPROPERTY(Transient)
	TObjectPtr<AActor> ActiveDrainTarget;

	FTimerHandle DrainTickTimerHandle;
	float DrainStartTime = 0.0f;
};
