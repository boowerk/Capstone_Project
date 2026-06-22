#pragma once

#include "CoreMinimal.h"
#include "Characters/GP_EnemyCharacter.h"
#include "TimerManager.h"
#include "GP_CrystalSeraphBossCharacter.generated.h"

class UGameplayAbility;
class UGP_CrystalSeraphStateComponent;
class FCrystalSeraphGroggyLifecycleTest;

UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_CrystalSeraphBossCharacter : public AGP_EnemyCharacter
{
	GENERATED_BODY()

public:
	AGP_CrystalSeraphBossCharacter();

	UFUNCTION(BlueprintPure, Category = "Boss|Crystal Seraph")
	UGP_CrystalSeraphStateComponent* GetCrystalSeraphStateComponent() const { return CrystalSeraphStateComponent; }

	UFUNCTION(BlueprintCallable, Category = "Boss|Crystal Seraph")
	AActor* RequestSpawnCrystalPrism(AActor* PatternTargetActor);

	UFUNCTION(BlueprintCallable, Category = "Boss|Crystal Seraph")
	bool RequestStartLaserPattern(AActor* PatternTargetActor);

	UFUNCTION(BlueprintCallable, Category = "Boss|Crystal Seraph")
	bool RequestSpawnCrystalShardPattern(AActor* PatternTargetActor);

	UFUNCTION(BlueprintCallable, Category = "Boss|Crystal Seraph")
	bool RequestStartAreaPattern(AActor* PatternTargetActor);

	UFUNCTION(BlueprintCallable, Category = "Boss|Crystal Seraph")
	void RequestExposeWingCore(float ExposureDurationOverride = -1.0f);

	UFUNCTION(BlueprintCallable, Category = "Boss|Crystal Seraph")
	void RequestWingCoreBreak();

	UFUNCTION(BlueprintCallable, Category = "Boss|Crystal Seraph")
	void RequestEnterGroggy();

	UFUNCTION(BlueprintCallable, Category = "Boss|Crystal Seraph")
	void RequestRecoverFromGroggy();

	UFUNCTION(BlueprintCallable, Category = "Boss|Crystal Seraph")
	bool RequestTeleportToPreferredCombatPosition(AActor* PatternTargetActor);

	UFUNCTION(BlueprintPure, Category = "Boss|Crystal Seraph")
	float GetPreferredHoverHeight() const { return FMath::Max(0.0f, PreferredHoverHeight); }

	UFUNCTION(BlueprintPure, Category = "Boss|Crystal Seraph")
	float GetPreferredAirRange() const { return FMath::Max(0.0f, PreferredAirRange); }

	UFUNCTION(BlueprintPure, Category = "Boss|Crystal Seraph")
	bool ShouldTeleportForCrystalSeraph(float DistanceToTarget) const;

	UFUNCTION(BlueprintPure, Category = "Boss|Crystal Seraph")
	float GetLaserPatternCooldown() const { return FMath::Max(0.0f, LaserPatternCooldown); }

	UFUNCTION(BlueprintPure, Category = "Boss|Crystal Seraph")
	float GetPrismPatternCooldown() const { return FMath::Max(0.0f, PrismPatternCooldown); }

	UFUNCTION(BlueprintPure, Category = "Boss|Crystal Seraph")
	float GetLastLaserPatternTime() const { return LastLaserPatternTime; }

	UFUNCTION(BlueprintPure, Category = "Boss|Crystal Seraph")
	float GetLastPrismPatternTime() const { return LastPrismPatternTime; }

	UFUNCTION(BlueprintPure, Category = "Boss|Crystal Seraph")
	bool CanStartCrystalSeraphPattern() const;

	UFUNCTION(BlueprintPure, Category = "Boss|Crystal Seraph")
	bool IsGroggyRecoveryScheduled() const { return bGroggyRecoveryScheduled; }

	// Reserves the shared attack cadence before a GAS pattern executes so back-to-back BT tasks cannot pass the same check.
	bool TryStartCrystalSeraphPattern();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void HandlePostDamageTaken(AActor* InstigatorActor, float DamageAmount, FGameplayTag ElementTag) override;

private:
	friend class FCrystalSeraphGroggyLifecycleTest;

	UFUNCTION()
	void HandleCrystalSeraphGroggyChanged(bool bNewGroggy);

	UFUNCTION()
	void HandleCrystalSeraphWingCoreExposedChanged(bool bNewExposed);

	void GrantCrystalSeraphPatternAbilities();
	AActor* ResolvePatternTarget(AActor* ExplicitTargetActor) const;
	FVector ResolvePrismSpawnLocation(AActor* TargetActor, int32 PrismIndex, int32 TotalPrismCount) const;
	FVector ResolveHoverLocation() const;
	FVector ConstrainTacticalDestinationToAnchor(const FVector& DesiredDestination) const;
	float ResolveBossMaxHealth() const;
	float ResolveCurrentGroggyDuration() const;
	bool IsPlayerDamageInstigator(const AActor* InstigatorActor) const;
	void ScheduleGroggyRecoveryAfterPlayerHit(AActor* InstigatorActor, float DamageAmount);
	void MoveToHoverLocation();
	AActor* SpawnConfiguredActor(TSubclassOf<AActor> ActorClass, const FVector& Location, const FRotator& Rotation, FName DebugName);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGP_CrystalSeraphStateComponent> CrystalSeraphStateComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Crystal Seraph|Actors", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AActor> CrystalPrismActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Crystal Seraph|Actors", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AActor> SeraphLaserActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Crystal Seraph|Actors", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AActor> WingCoreHitActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Crystal Seraph|Actors", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AActor> CrystalShardProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Crystal Seraph|Actors", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AActor> AreaMarkerActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Crystal Seraph|Abilities", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayAbility> CrystalShardAbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Crystal Seraph|Abilities", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayAbility> CrystalLaserAbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Crystal Seraph|Abilities", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayAbility> CrystalPrismAbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Crystal Seraph|Abilities", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayAbility> CrystalAreaAbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Crystal Seraph|Abilities", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayAbility> CrystalGroggyAbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Crystal Seraph|Abilities", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayAbility> CrystalTeleportAbilityClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph|Abilities", meta = (AllowPrivateAccess = "true"))
	bool bGrantCrystalSeraphPatternAbilities = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph|Air", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float PreferredHoverHeight = 650.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph|Air", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float PreferredAirRange = 1100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph|Air", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float CloseTeleportDistance = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph|Air", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float FarTeleportDistance = 1900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph|Air", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float TacticalTeleportCooldown = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph|Air", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float TacticalTeleportAnchorBuffer = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph|Combat", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float MinimumPatternInterval = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph|Prism", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float PrismPatternCooldown = 16.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph|Prism", meta = (AllowPrivateAccess = "true", ClampMin = "1", ClampMax = "8"))
	int32 PrismSpawnCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph|Prism", meta = (AllowPrivateAccess = "true", ClampMin = "200.0", Units = "cm"))
	float PrismRingRadius = 650.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph|Laser", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float LaserPatternCooldown = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph|Core", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float WingCoreExposureDuration = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph|Core", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float PhaseTwoWingCoreExposureDuration = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph|Groggy", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float GroggyDuration = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph|Groggy", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float FinalPhaseGroggyDuration = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph|Phase", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0"))
	float PhaseTwoHealthRatio = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph|Phase", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0"))
	float FinalPhaseHealthRatio = 0.25f;

	float LastLaserPatternTime = -BIG_NUMBER;
	float LastPrismPatternTime = -BIG_NUMBER;
	float LastPatternStartTime = -BIG_NUMBER;
	float LastTacticalTeleportTime = -BIG_NUMBER;
	int32 TacticalTeleportSequence = 0;
	FTimerHandle GroggyRecoveryTimerHandle;
	bool bGroggyRecoveryScheduled = false;
};
