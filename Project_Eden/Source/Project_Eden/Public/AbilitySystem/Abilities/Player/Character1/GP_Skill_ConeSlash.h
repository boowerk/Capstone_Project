#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_SkillBase.h"
#include "GameplayTagContainer.h"
#include "GP_Skill_ConeSlash.generated.h"

class AActor;

/**
 * Hits enemies in an instant forward cone.
 */
UCLASS()
class PROJECT_EDEN_API UGP_Skill_ConeSlash : public UGP_SkillBase
{
	GENERATED_BODY()

public:
	UGP_Skill_ConeSlash();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|ConeSlash|Visuals")
	TSubclassOf<AActor> SlashVisualActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|ConeSlash", meta = (ClampMin = "0.0"))
	float Range = 450.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|ConeSlash", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float ConeAngle = 70.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|ConeSlash")
	float VisualForwardOffset = 220.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|ConeSlash")
	float VisualHeightOffset = 80.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|ConeSlash")
	FGameplayTag HitEventTag;
};
