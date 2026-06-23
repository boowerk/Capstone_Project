#pragma once

#include "CoreMinimal.h"
#include "Characters/GP_EnemyCharacter.h"
#include "GP_DarkArmorKnightBossCharacter.generated.h"

class AGP_DarkKnightChargeActor;
class AGP_DarkKnightGroundCrackActor;
class AGP_DarkWaveProjectile;
class UGameplayAbility;
class UGameplayEffect;
class UAnimMontage;
class UGP_BossTelegraphVFXComponent;
class UGP_DarkArmorKnightStateComponent;

UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_DarkArmorKnightBossCharacter : public AGP_EnemyCharacter
{
	GENERATED_BODY()

public:
	AGP_DarkArmorKnightBossCharacter();

	UFUNCTION(BlueprintPure, Category = "Boss|Dark Knight")
	UGP_DarkArmorKnightStateComponent* GetDarkKnightStateComponent() const { return DarkKnightStateComponent; }

	UFUNCTION(BlueprintPure, Category = "Boss|Dark Knight|VFX")
	UGP_BossTelegraphVFXComponent* GetBossTelegraphVFXComponent() const;

	UFUNCTION(BlueprintPure, Category = "Boss|Dark Knight")
	int32 GetDarkKnightPhase() const;

	UFUNCTION(BlueprintPure, Category = "Boss|Dark Knight")
	bool CanStartDarkKnightPattern() const;

	bool TryStartDarkKnightPattern();

	UFUNCTION(BlueprintPure, Category = "Boss|Dark Knight")
	bool IsPatternCooldownReady(FGameplayTag PatternTag) const;

	UFUNCTION(BlueprintCallable, Category = "Boss|Dark Knight")
	void RequestCounterAttackAbility(AActor* CounterTarget);

	UFUNCTION(BlueprintCallable, Category = "Boss|Dark Knight")
	void RequestEnterGroggyAbility();

	bool ExecuteBasicAttack(AActor* TargetActor);
	bool ExecuteHeavyAttack(AActor* TargetActor);
	bool ExecuteSweepAttack(AActor* TargetActor);
	bool ExecuteGuardStance();
	bool ExecuteCounterAttack(AActor* TargetActor);
	bool ExecuteChargeAttack(AActor* TargetActor);
	bool ExecuteDarkWave(AActor* TargetActor);
	bool ExecuteGroundCrack(AActor* TargetActor);
	bool ExecuteEnterGroggy();

	/** Plays the shared ready stance before a single authored attack. */
	bool StartPatternWithWindup(FGameplayTag PatternTag, AActor* TargetActor);

	/** Plays the authored montage for a successfully-started boss pattern. */
	bool PlayPatternMontage(FGameplayTag PatternTag);

	void HandleChargeFinished(bool bHitTarget, AActor* TargetActor);

	UFUNCTION(BlueprintPure, Category = "Boss|Dark Knight")
	float GetPreferredMeleeRange() const { return PreferredMeleeRange; }

	UFUNCTION(BlueprintPure, Category = "Boss|Dark Knight")
	float GetChargeMinRange() const { return ChargeMinRange; }

	UFUNCTION(BlueprintPure, Category = "Boss|Dark Knight")
	float GetDarkWaveMaxRange() const { return DarkWaveMaxRange; }

	UFUNCTION(BlueprintPure, Category = "Boss|Dark Knight")
	float GetGroundCrackMaxRange() const { return GroundCrackMaxRange; }

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void HandlePostDamageTaken(AActor* InstigatorActor, float DamageAmount, FGameplayTag ElementTag) override;

private:
	void GrantDarkKnightAbilities();
	AActor* ResolvePatternTarget(AActor* ExplicitTarget) const;
	TArray<AActor*> GatherTargetsInCone(float Range, float HalfAngleDegrees) const;
	void ApplyConeDamage(float Range, float HalfAngleDegrees, float DamageCoefficient, float KnockbackStrength = 0.0f);
	void RecordPatternUse(const FGameplayTag& PatternTag);
	float ResolvePatternCooldown(const FGameplayTag& PatternTag) const;
	void SpawnDarkWaveVolley(AActor* TargetActor, int32 ProjectileCount);
	void SpawnGroundCracks(AActor* TargetActor);
	bool ExecutePatternNow(FGameplayTag PatternTag, AActor* TargetActor);

	UFUNCTION()
	void HandleGroggyChanged(bool bNewGroggy);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGP_DarkArmorKnightStateComponent> DarkKnightStateComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Dark Knight|Actors", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AGP_DarkWaveProjectile> DarkWaveProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Dark Knight|Actors", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AGP_DarkKnightGroundCrackActor> GroundCrackActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Dark Knight|Actors", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AGP_DarkKnightChargeActor> ChargeActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Dark Knight|GAS", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Dark Knight|Abilities", meta = (AllowPrivateAccess = "true"))
	TArray<TSubclassOf<UGameplayAbility>> DarkKnightAbilityClasses;

	/** Keyed by GPTags::Ability::Boss::DarkKnight::* so Blueprint children can swap visuals without changing pattern logic. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Dark Knight|Animation", meta = (AllowPrivateAccess = "true"))
	TMap<FGameplayTag, TObjectPtr<UAnimMontage>> PatternMontages;

	/** Shared pre-attack stance; actual strike montage starts after this readable cue. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Dark Knight|Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> PreAttackMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Dark Knight|Animation", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float PreAttackDuration = 1.18f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight|Combat", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float PreferredMeleeRange = 350.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight|Combat", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float MinimumPatternInterval = 1.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight|Basic", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float BasicAttackRange = 350.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight|Guard", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float GuardDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight|Guard", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float ParryWindowDuration = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight|Groggy", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float GroggyDuration = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight|Groggy", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float FinalPhaseGroggyDuration = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight|Charge", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float ChargeMinRange = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight|Dark Wave", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float DarkWaveMaxRange = 2200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight|Ground Crack", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float GroundCrackMaxRange = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight|Cooldown", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float BasicCooldown = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight|Cooldown", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float HeavyCooldown = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight|Cooldown", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float SweepCooldown = 7.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight|Cooldown", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float GuardCooldown = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight|Cooldown", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float ChargeCooldown = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight|Cooldown", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float DarkWaveCooldown = 9.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight|Cooldown", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float GroundCrackCooldown = 14.0f;

	float LastPatternStartTime = -BIG_NUMBER;
	TMap<FGameplayTag, float> LastPatternUseTimes;
	TArray<FTimerHandle> PatternTimerHandles;
	TWeakObjectPtr<AActor> PendingCounterTarget;
};
