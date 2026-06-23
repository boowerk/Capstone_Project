#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_TargetedSkillBase.h"
#include "GameplayTagContainer.h"
#include "GP_Skill_LightningStrike.generated.h"

class AActor;

/**
 * Ground-targeted area strike confirmed when the equipped skill input is released.
 */
UCLASS()
class PROJECT_EDEN_API UGP_Skill_LightningStrike : public UGP_TargetedSkillBase
{
	GENERATED_BODY()

public:
	UGP_Skill_LightningStrike();

protected:
	virtual void ExecuteConfirmedSkill_Implementation(const FGP_SkillTargetData& TargetData) override;
	virtual float GetPreviewActorRadius() const override;

	float GetFinalStrikeRadius(const UGP_SkillData* SkillData) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|LightningStrike|Visuals")
	TSubclassOf<AActor> StrikeVisualActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|LightningStrike", meta = (ClampMin = "0.0"))
	float StrikeRadius = 300.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|LightningStrike")
	FGameplayTag HitEventTag;
};
