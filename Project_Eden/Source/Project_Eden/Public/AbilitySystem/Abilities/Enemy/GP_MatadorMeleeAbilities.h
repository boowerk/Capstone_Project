#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_SkillBase.h"
#include "GameplayTagContainer.h"
#include "TimerManager.h"
#include "GP_MatadorMeleeAbilities.generated.h"

class UGameplayEffect;
class UAbilityTask_PlayMontageAndWait;

UCLASS(Abstract, Blueprintable)
class PROJECT_EDEN_API UGP_MatadorMeleeAbilityBase : public UGP_SkillBase
{
	GENERATED_BODY()

public:
	UGP_MatadorMeleeAbilityBase();

	virtual bool CheckCooldown(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ApplyCooldown(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	AActor* ResolveTargetActor(AActor* AvatarActor) const;
	AActor* ResolvePatternActor(AActor* AbilityAvatarActor) const;
	bool RotateAvatarTowardTarget(AActor* AvatarActor, AActor* TargetActor, float DeltaSeconds, float TurnRateDegreesPerSecond) const;
	void StopAIMovement(AActor* AvatarActor) const;
	void PlayConfiguredMontage();
	void ApplySetByCallerDamage(AActor* AvatarActor, const TArray<AActor*>& TargetActors, float BaseDamage, float ToughnessDamage, float AttackPowerCoefficient) const;
	void ApplyEffectToTargets(AActor* AvatarActor, const TArray<AActor*>& TargetActors, TSubclassOf<UGameplayEffect> EffectClass) const;
	float ResolveBlackboardHealthRatio(AActor* AvatarActor) const;
	bool TryStartMeleeLock(const FGameplayAbilityActorInfo* ActorInfo);
	void ClearOwnedTimers();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Cooldown")
	FGameplayTag CooldownTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Cooldown", meta = (ClampMin = "0.0", Units = "s"))
	float CooldownDuration = 6.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Damage", meta = (ClampMin = "0.0"))
	float FallbackBaseDamage = 12.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Damage", meta = (ClampMin = "0.0"))
	float FallbackToughnessDamage = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Damage", meta = (ClampMin = "0.0"))
	float FallbackAttackPowerCoefficient = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Facing", meta = (ClampMin = "1.0", Units = "deg/s"))
	float AimTurnRateDegreesPerSecond = 240.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Hit", meta = (ClampMin = "0.0", Units = "cm"))
	float HitBoxElevationOffset = 45.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Hit")
	bool bDebugHitShape = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Telegraph")
	bool bShowTemporaryTelegraph = true;

	FTimerHandle PrimaryTimerHandle;
	FTimerHandle SecondaryTimerHandle;
	FTimerHandle TertiaryTimerHandle;
	bool bMeleeLockActive = false;
};

UCLASS(Blueprintable)
class PROJECT_EDEN_API UGP_MatadorRapierThrustAbility : public UGP_MatadorMeleeAbilityBase
{
	GENERATED_BODY()

public:
	UGP_MatadorRapierThrustAbility();

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

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Matador|Rapier")
	void BP_OnRapierAimStarted(AActor* AvatarActor, float InAimDuration);

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Matador|Rapier")
	void BP_OnRapierGlowUpdated(AActor* AvatarActor, float GlowAlpha);

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Matador|Rapier")
	void BP_OnRapierDirectionLocked(AActor* AvatarActor, FVector LockedDirection, float InCommitDelay);

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Matador|Rapier")
	void BP_OnRapierThrust(AActor* AvatarActor, FVector LockedDirection);

private:
	void TickAim();
	void LockDirection();
	void PerformThrust();
	void FinishAfterThrust();
	void SetRapierGlow(AActor* AvatarActor, float GlowAlpha) const;
	void DrawRapierTelegraph(AActor* AvatarActor, const FVector& Direction, FColor Color, float Duration) const;

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Rapier", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float AimDuration = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Rapier", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float CommitDelay = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Rapier", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float ThrustRange = 900.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Rapier", meta = (AllowPrivateAccess = "true", ClampMin = "1.0", Units = "cm"))
	float ThrustHalfWidth = 55.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Rapier", meta = (AllowPrivateAccess = "true", ClampMin = "1.0", Units = "cm"))
	float ThrustHalfHeight = 80.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Rapier", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float PostThrustRecovery = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Rapier", meta = (AllowPrivateAccess = "true"))
	FName RapierGlowParameterName = TEXT("RapierGlowIntensity");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Rapier", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float MaxRapierGlowValue = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Rapier|Telegraph", meta = (AllowPrivateAccess = "true"))
	FColor RapierAimTelegraphColor = FColor(255, 220, 60);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Rapier|Telegraph", meta = (AllowPrivateAccess = "true"))
	FColor RapierLockedTelegraphColor = FColor(255, 30, 30);

	float AimElapsedTime = 0.0f;
	FVector LockedThrustDirection = FVector::ForwardVector;
};

UCLASS(Blueprintable)
class PROJECT_EDEN_API UGP_MatadorCapeGustAbility : public UGP_MatadorMeleeAbilityBase
{
	GENERATED_BODY()

public:
	UGP_MatadorCapeGustAbility();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Matador|Cape")
	void BP_OnCapePrepareStarted(AActor* AvatarActor, float InPrepareDuration);

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Matador|Cape")
	void BP_OnCapeDirectionLocked(AActor* AvatarActor, FVector LockedDirection);

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Matador|Cape")
	void BP_OnCapeGustBurst(AActor* AvatarActor, FVector LockedDirection, int32 InBurstIndex, int32 InBurstCount);

private:
	void TickPrepare();
	void LockDirectionAndStartBursts();
	void PerformGustBurst();
	void FinishCapeGust();
	void ApplyKnockback(AActor* AvatarActor, const TArray<AActor*>& HitActors) const;
	void DrawCapeTelegraph(AActor* AvatarActor, const FVector& Direction, FColor Color, float Duration) const;

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Cape", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float PrepareDuration = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Cape", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float GustRange = 700.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Cape", meta = (AllowPrivateAccess = "true", ClampMin = "1.0", ClampMax = "360.0", Units = "deg"))
	float GustArcAngleDegrees = 30.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Cape", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float GustForwardOffset = 260.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Cape", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm/s"))
	float KnockbackStrength = 1100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Cape", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm/s"))
	float KnockbackUpwardStrength = 80.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Cape", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> SlowEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Cape", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0"))
	float LowHealthThreshold = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Cape", meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 LowHealthBurstCount = 4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Cape", meta = (AllowPrivateAccess = "true", ClampMin = "0.01", Units = "s"))
	float LowHealthBurstInterval = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Cape", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float PostGustRecovery = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Cape|Telegraph", meta = (AllowPrivateAccess = "true"))
	FColor CapePrepareTelegraphColor = FColor(255, 80, 40);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Cape|Telegraph", meta = (AllowPrivateAccess = "true"))
	FColor CapeBurstTelegraphColor = FColor(255, 0, 0);

	float PrepareElapsedTime = 0.0f;
	FVector LockedGustDirection = FVector::ForwardVector;
	int32 BurstIndex = 0;
	int32 BurstCount = 1;
};
