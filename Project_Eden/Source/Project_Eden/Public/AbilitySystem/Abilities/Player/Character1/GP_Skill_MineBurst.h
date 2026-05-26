#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_SkillBase.h"
#include "GP_Skill_MineBurst.generated.h"

class AGP_MineBurstActor;

/**
 * Places a mine on the ground in front of the player.
 */
UCLASS()
class PROJECT_EDEN_API UGP_Skill_MineBurst : public UGP_SkillBase
{
	GENERATED_BODY()

public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|MineBurst")
	TSubclassOf<AGP_MineBurstActor> MineActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|MineBurst", meta = (ClampMin = "0.0"))
	float PlaceForwardOffset = 300.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|MineBurst", meta = (ClampMin = "0.0"))
	float TraceHeight = 300.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|MineBurst", meta = (ClampMin = "0.0"))
	float TraceDepth = 700.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|MineBurst")
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;
};
