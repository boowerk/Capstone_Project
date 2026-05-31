#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_SkillBase.h"
#include "GameplayTagContainer.h"
#include "TimerManager.h"
#include "GP_Skill_DashSlash.generated.h"

class AActor;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;

/**
 * Root-motion sword dash that applies a forward slash hit during the montage.
 */
UCLASS()
class PROJECT_EDEN_API UGP_Skill_DashSlash : public UGP_SkillBase
{
	GENERATED_BODY()

public:
	UGP_Skill_DashSlash();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual bool CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|DashSlash|Visuals")
	TSubclassOf<AActor> SlashVisualActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|DashSlash", meta = (ClampMin = "0.0"))
	float HitRange = 520.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|DashSlash", meta = (ClampMin = "0.0"))
	float HitWidth = 220.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|DashSlash", meta = (ClampMin = "0.0"))
	float HitHeight = 180.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|DashSlash")
	float HitHeightOffset = 80.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|DashSlash")
	float VisualForwardOffset = 260.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|DashSlash")
	float VisualHeightOffset = 90.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|DashSlash")
	FGameplayTag HitEventTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|DashSlash|Cooldown", meta = (ClampMin = "0.0"))
	float DefaultCooldownDuration = 5.0f;

private:
	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> WaitHitTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> WaitEndTask;

	FTimerHandle FallbackMontageEndTimerHandle;
	FTimerHandle FallbackAttackHitTimerHandle;
	FTimerHandle FallbackActionEndTimerHandle;
	FDelegateHandle ActionRootMotionCancelInputHandle;

	bool bHasAppliedAttackHit = false;
	bool bMovementControlUnlocked = false;

	void ClearExistingTasks();
	void PerformDashSlashHit();
	void UnlockMovementControl();

	UFUNCTION()
	void OnAttackHitEventReceived(FGameplayEventData Payload);

	UFUNCTION()
	void OnActionEndEventReceived(FGameplayEventData Payload);

	UFUNCTION()
	void OnFallbackActionEnd();

	void OnActionRootMotionCancelInput();

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageBlendOut();

	UFUNCTION()
	void OnMontageInterrupted();
};
