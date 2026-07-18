#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_SkillBase.h"
#include "GameplayTagContainer.h"
#include "GP_EnemyAttack.generated.h"

class UGameplayEffect;
class UAnimMontage;
class UCharacterMovementComponent;
class UCapsuleComponent;
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

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	// Derived enemy archetypes can replace the shared overlap hit while keeping montage and gameplay-event timing.
	virtual void PerformAttackHit();

	// Animation notifies can send this event when a montage should apply the hit.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Mechanics")
	FGameplayTag AttackEventTag;

	// Animation notifies can send this event when a montage has reached its actionable end.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Mechanics")
	FGameplayTag ActionEndEventTag;

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

	UFUNCTION()
	void OnActionEndEventReceived(FGameplayEventData Payload);

	void FinishAttackAbility(bool bWasCancelled);
	UFUNCTION() void BeginAbilityForwardStep();
	void TickAbilityForwardStep();
	void EndAbilityForwardStep();
	void PushOverlappingForwardStepTargets();

	bool bHasAppliedAttackHit = false;
	bool bHasFinishedAttackAbility = false;

	// Per-actor ability instance state. Prevents the same configured attack montage repeating twice in a row.
	TObjectPtr<UAnimMontage> LastSelectedAttackMontage = nullptr;

	// -1 = left, 1 = right, 0 = unknown. Used to prefer left/right alternation for named attack montages.
	int8 LastSelectedAttackSide = 0;
	FVector AbilityForwardStepDirection = FVector::ZeroVector;
	float AbilityForwardStepDistance = 0.0f;
	float AbilityForwardStepDurationSeconds = 0.0f;
	float AbilityForwardStepPushSpeed = 0.0f;
	bool bAbilityForwardStepActive = false;
	TObjectPtr<UCharacterMovementComponent> AbilityForwardStepMovementComponent = nullptr;
	TObjectPtr<UCapsuleComponent> AbilityForwardStepCapsuleComponent = nullptr;
	ECollisionResponse AbilityForwardStepPreviousPawnResponse = ECR_Block;
	TSet<TWeakObjectPtr<AActor>> AbilityForwardStepPushedActors;
	FTimerHandle AbilityForwardStepTimerHandle;
};
