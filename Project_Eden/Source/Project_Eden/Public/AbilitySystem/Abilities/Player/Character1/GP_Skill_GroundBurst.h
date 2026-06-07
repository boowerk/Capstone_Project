#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_SkillBase.h"
#include "GameplayTagContainer.h"
#include "GP_Skill_GroundBurst.generated.h"

class AActor;

/**
 * Bursts at a ground position in front of the player's aim direction.
 */
UCLASS()
class PROJECT_EDEN_API UGP_Skill_GroundBurst : public UGP_SkillBase
{
	GENERATED_BODY()

public:
	UGP_Skill_GroundBurst();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|GroundBurst|Visuals")
	TSubclassOf<AActor> BurstVisualActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|GroundBurst", meta = (ClampMin = "0.0"))
	float TargetForwardOffset = 500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|GroundBurst", meta = (ClampMin = "0.0"))
	float BurstRadius = 300.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|GroundBurst", meta = (ClampMin = "0.0"))
	float TraceHeight = 500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|GroundBurst", meta = (ClampMin = "0.0"))
	float TraceDepth = 1000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|GroundBurst")
	FGameplayTag HitEventTag;
};
