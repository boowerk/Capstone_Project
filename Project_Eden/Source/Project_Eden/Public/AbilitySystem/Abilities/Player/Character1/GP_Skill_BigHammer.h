#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_TargetedSkillBase.h"
#include "GameplayTagContainer.h"
#include "GP_Skill_BigHammer.generated.h"

class AGP_BigHammerDropActor;

/**
 * Ground-targeted falling hammer with area damage on impact.
 */
UCLASS()
class PROJECT_EDEN_API UGP_Skill_BigHammer : public UGP_TargetedSkillBase
{
	GENERATED_BODY()

public:
	UGP_Skill_BigHammer();

protected:
	virtual void ExecuteConfirmedSkill_Implementation(const FGP_SkillTargetData& TargetData) override;
	virtual float GetPreviewActorRadius() const override;

	float GetFinalImpactRadius(const UGP_SkillData* SkillData) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|BigHammer|Actor")
	TSubclassOf<AGP_BigHammerDropActor> HammerDropActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|BigHammer", meta = (ClampMin = "0.0"))
	float ImpactRadius = 300.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|BigHammer|Movement", meta = (ClampMin = "0.0", Units = "cm"))
	float DropHeight = 600.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|BigHammer|Movement", meta = (ClampMin = "0.01", Units = "s"))
	float DropDuration = 0.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|BigHammer|Movement", meta = (Units = "cm", ToolTip = "Offsets the hammer actor origin at impact so the visible hammer head touches the ground."))
	float ImpactZOffset = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|BigHammer|Movement")
	FRotator HammerRotation = FRotator(180.0f, 0.0f, 0.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|BigHammer")
	FGameplayTag HitEventTag;
};
