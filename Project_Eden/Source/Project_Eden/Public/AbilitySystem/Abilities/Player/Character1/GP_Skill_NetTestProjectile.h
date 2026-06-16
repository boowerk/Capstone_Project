// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_TargetedSkillBase.h"
#include "GP_Skill_NetTestProjectile.generated.h"

class AGP_Projectile;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;

/**
 * 네트워크 테스트 프로젝타일 스킬
 */
UCLASS()
class PROJECT_EDEN_API UGP_Skill_NetTestProjectile : public UGP_TargetedSkillBase
{
	GENERATED_BODY()

public:
	UGP_Skill_NetTestProjectile();

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	virtual void ExecuteConfirmedSkill_Implementation(const FGP_SkillTargetData& TargetData) override;

	// --- 수치 및 설정 (Values) ---

	// 프로젝타일 클래스 (블루프린트에서 설정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Projectile")
	TSubclassOf<AGP_Projectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Projectile")
	float SpawnForwardOffset = 120.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Projectile")
	float SpawnUpOffset = 60.f;

private:
	void SpawnProjectiles(const FGP_SkillTargetData& TargetData);
	void EndAfterProjectileMontage(bool bWasCancelled);

	UFUNCTION()
	void OnAttackHitEventReceived(FGameplayEventData Payload);

	UFUNCTION()
	void OnMontageFinished();

	UFUNCTION()
	void OnMontageInterrupted();

	void OnFallbackAttackHit();
	void OnFallbackMontageFinished();
	void ClearProjectileMontageState();

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> WaitHitTask;

	FGP_SkillTargetData PendingTargetData;
	FTimerHandle FallbackAttackHitTimerHandle;
	FTimerHandle FallbackMontageEndTimerHandle;
	bool bHasSpawnedProjectiles = false;
	bool bPlayingSourceFallbackMontage = false;
};
