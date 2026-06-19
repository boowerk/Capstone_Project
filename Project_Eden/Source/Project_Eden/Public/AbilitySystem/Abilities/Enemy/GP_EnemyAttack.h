#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_SkillBase.h"
#include "GameplayTagContainer.h"
#include "GP_EnemyAttack.generated.h"

class UGameplayEffect;
struct FGameplayEventData;

UCLASS()
class PROJECT_EDEN_API UGP_EnemyAttack : public UGP_SkillBase
{
	GENERATED_BODY()

public:
	UGP_EnemyAttack();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// Derived enemy archetypes can replace the shared overlap hit while keeping montage and gameplay-event timing.
	virtual void PerformAttackHit();

	// Animation notifies can send this event when a montage should apply the hit.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Mechanics")
	FGameplayTag AttackEventTag;

	// Generic enemies use event timing by default; boss abilities can override their own timing in derived classes.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Mechanics")
	bool bUseGameplayEventForHitTiming = true;

	// Shared vertical offset kept for existing enemy and boss ability tuning.
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Skill|Values")
	float HitBoxElevationOffset = 40.0f;

private:
	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnAttackEventReceived(FGameplayEventData Payload);

	bool bHasAppliedAttackHit = false;
};
