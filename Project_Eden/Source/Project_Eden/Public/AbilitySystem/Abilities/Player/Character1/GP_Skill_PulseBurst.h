#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_SkillBase.h"
#include "GameplayTagContainer.h"
#include "GP_Skill_PulseBurst.generated.h"

class AActor;

/**
 * Bursts immediately around the player.
 */
UCLASS()
class PROJECT_EDEN_API UGP_Skill_PulseBurst : public UGP_SkillBase
{
	GENERATED_BODY()

public:
	UGP_Skill_PulseBurst();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|PulseBurst|Visuals")
	TSubclassOf<AActor> BurstVisualActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|PulseBurst", meta = (ClampMin = "0.0"))
	float BurstRadius = 250.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|PulseBurst")
	FGameplayTag HitEventTag;
};
