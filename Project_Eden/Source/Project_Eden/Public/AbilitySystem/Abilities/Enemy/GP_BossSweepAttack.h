#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Enemy/GP_EnemyAttack.h"
#include "TimerManager.h"
#include "GP_BossSweepAttack.generated.h"

class AGP_BossSweepTelegraphActor;
class UAnimMontage;
class UMaterialInterface;
struct FGameplayEventData;

UCLASS(Blueprintable)
class PROJECT_EDEN_API UGP_BossSweepAttack : public UGP_EnemyAttack
{
	GENERATED_BODY()

public:
	UGP_BossSweepAttack();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

private:
	UFUNCTION()
	void OnBossSweepMontageCompleted();

	bool StartBossSweepTelegraph(AActor* AvatarActor);
	void OnBossSweepTelegraphElapsed();
	void BeginBossSweepAttack();
	void SpawnBossSweepTelegraph(AActor* AvatarActor);
	void PerformBossSweepHit();

private:
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Sweep")
	bool bShowBossSweepTelegraph = true;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Sweep", meta = (EditCondition = "bShowBossSweepTelegraph", ClampMin = "0.0"))
	float BossSweepTelegraphDelay = 0.85f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Sweep", meta = (EditCondition = "bShowBossSweepTelegraph"))
	FLinearColor BossSweepTelegraphColor = FLinearColor(1.0f, 0.08f, 0.02f, 0.48f);

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Sweep", meta = (EditCondition = "bShowBossSweepTelegraph"))
	TObjectPtr<UMaterialInterface> BossSweepTelegraphMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Sweep", meta = (ClampMin = "1.0", ClampMax = "360.0", Units = "deg"))
	float BossSweepArcAngleDegrees = 165.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Sweep", meta = (ClampMin = "0.0", Units = "cm"))
	float BossSweepHitBoxElevationOffset = 40.0f;

	bool bBossSweepHitApplied = false;
	bool bWaitingForBossSweepTelegraph = false;
	bool bBossSweepMontageStarted = false;
	FTimerHandle BossSweepTelegraphTimerHandle;

	UPROPERTY(Transient)
	TObjectPtr<AGP_BossSweepTelegraphActor> ActiveBossSweepTelegraph;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> PendingBossSweepMontage;
};
