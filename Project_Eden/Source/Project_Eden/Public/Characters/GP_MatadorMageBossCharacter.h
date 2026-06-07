#pragma once

#include "CoreMinimal.h"
#include "Characters/GP_EnemyCharacter.h"
#include "TimerManager.h"
#include "GP_MatadorMageBossCharacter.generated.h"

class AGP_BullChargeActor;
class AGP_ChainEffectActor;
class AGP_MatadorBossDecoyActor;
class UGameplayAbility;
class UGP_MatadorBossStateComponent;

UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_MatadorMageBossCharacter : public AGP_EnemyCharacter
{
	GENERATED_BODY()

public:
	AGP_MatadorMageBossCharacter();

	UFUNCTION(BlueprintPure, Category = "Boss|Matador")
	UGP_MatadorBossStateComponent* GetMatadorStateComponent() const { return MatadorStateComponent; }

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador")
	AGP_MatadorBossDecoyActor* EnsureMatadorDecoy();

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador")
	AGP_ChainEffectActor* EnsureChainEffect();

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador")
	AGP_BullChargeActor* SpawnBullPattern(AActor* PatternTargetActor);

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador")
	bool RequestStartBullPattern(AActor* PatternTargetActor);

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador")
	void RequestEnterGroggy();

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador")
	void RequestRecoverFromGroggy();

	UFUNCTION(BlueprintPure, Category = "Boss|Matador")
	float GetPreferredHoverHeight() const { return FMath::Max(0.0f, PreferredHoverHeight); }

	UFUNCTION(BlueprintPure, Category = "Boss|Matador")
	float GetPreferredAirRange() const { return FMath::Max(0.0f, PreferredAirRange); }

	UFUNCTION(BlueprintPure, Category = "Boss|Matador")
	bool IsBullPatternActive() const;

	UFUNCTION(BlueprintPure, Category = "Boss|Matador")
	bool ShouldTeleportForMatador(float DistanceToTarget) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleMatadorChainStageChanged(int32 ChainBreakCount, int32 ChainBreakTarget);

	UFUNCTION()
	void HandleMatadorGroggyChanged(bool bNewGroggy);

	void GrantMatadorPatternAbilities();
	void HandleMatadorFallbackPatternTick();
	AActor* ResolvePatternTarget(AActor* ExplicitTargetActor) const;
	FVector ResolveDecoySpawnLocation(AActor* TargetActor) const;
	FVector ResolveBullSpawnLocation(AActor* TargetActor) const;
	FRotator ResolveFacingRotation(const FVector& FromLocation, const FVector& ToLocation) const;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Matador", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGP_MatadorBossStateComponent> MatadorStateComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Actors", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AGP_MatadorBossDecoyActor> DecoyActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Actors", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AGP_ChainEffectActor> ChainEffectActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Actors", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AGP_BullChargeActor> BullChargeActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Abilities", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayAbility> MatadorBullPatternAbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Abilities", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayAbility> MatadorGroggyAbilityClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador", meta = (AllowPrivateAccess = "true"))
	bool bAutoSpawnDecoyOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador", meta = (AllowPrivateAccess = "true"))
	bool bGrantMatadorPatternAbilities = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|Placement", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float DecoyForwardOffset = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|Placement", meta = (AllowPrivateAccess = "true", Units = "cm"))
	float DecoySideOffset = 420.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|Placement", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float BullSpawnDistanceFromTarget = 1350.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|Placement", meta = (AllowPrivateAccess = "true", Units = "cm"))
	float BullSpawnHeightOffset = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|Air", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float PreferredHoverHeight = 650.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|Air", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float PreferredAirRange = 1100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|Air", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float TeleportDistanceThreshold = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|Groggy", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float GroggyDuration = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|AI", meta = (AllowPrivateAccess = "true"))
	bool bUseFallbackMatadorPatternLoop = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|AI", meta = (AllowPrivateAccess = "true", ClampMin = "0.1", Units = "s"))
	float FallbackPatternInterval = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|AI", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float FallbackBullMinRange = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|AI", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float FallbackBullMaxRange = 2400.0f;

	FTimerHandle GroggyRecoveryTimerHandle;
	FTimerHandle FallbackPatternTimerHandle;
};
