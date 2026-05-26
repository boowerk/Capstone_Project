#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_SkillBase.h"
#include "GameplayTagContainer.h"
#include "GP_Skill_LineShock.generated.h"

class AActor;

/**
 * Hits enemies in an instant forward box.
 */
UCLASS()
class PROJECT_EDEN_API UGP_Skill_LineShock : public UGP_SkillBase
{
	GENERATED_BODY()

public:
	UGP_Skill_LineShock();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|LineShock|Visuals")
	TSubclassOf<AActor> ShockVisualActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|LineShock", meta = (ClampMin = "0.0"))
	float Range = 700.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|LineShock", meta = (ClampMin = "0.0"))
	float Width = 160.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|LineShock", meta = (ClampMin = "0.0"))
	float Height = 160.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|LineShock")
	float HeightOffset = 80.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|LineShock")
	FGameplayTag HitEventTag;
};
