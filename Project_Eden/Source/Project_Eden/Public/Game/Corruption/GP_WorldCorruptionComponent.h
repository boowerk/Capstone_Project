#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GP_WorldCorruptionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FGPOnWorldCorruptionChanged,
	float, Corruption,
	float, NormalizedCorruption);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FGPOnRegionCorruptionChanged,
	int32, RegionId,
	float, Corruption,
	float, NormalizedCorruption);

/**
 * Server-authoritative corruption domain state owned by GP_GameState.
 * Regional values are the source of truth; world corruption is their replicated average.
 */
UCLASS(ClassGroup = (World), meta = (BlueprintSpawnableComponent))
class PROJECT_EDEN_API UGP_WorldCorruptionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGP_WorldCorruptionComponent();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "World Corruption")
	float GetWorldCorruption() const { return WorldCorruption; }

	UFUNCTION(BlueprintPure, Category = "World Corruption")
	float GetWorldCorruptionNormalized() const;

	UFUNCTION(BlueprintPure, Category = "World Corruption")
	float GetRegionCorruption(int32 RegionId) const;

	UFUNCTION(BlueprintPure, Category = "World Corruption")
	float GetRegionCorruptionNormalized(int32 RegionId) const;

	UFUNCTION(BlueprintPure, Category = "World Corruption")
	int32 GetRegionCount() const { return RegionCorruptions.Num(); }

	UFUNCTION(BlueprintPure, Category = "World Corruption")
	float GetMaximumCorruption() const { return MaximumCorruption; }

	// GameMode initializes this once on the server after region count is known.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "World Corruption")
	void InitializeCorruption(
		int32 RegionCount,
		float InitialCorruption,
		float InMaximumCorruption,
		float InPassiveIncreasePerMinute,
		float InPassiveTickInterval,
		bool bEnablePassiveIncrease);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "World Corruption")
	void SetWorldCorruption(float NewCorruption);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "World Corruption")
	void AddWorldCorruption(float DeltaCorruption);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "World Corruption")
	void SetRegionCorruption(int32 RegionId, float NewCorruption);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "World Corruption")
	void AddRegionCorruption(int32 RegionId, float DeltaCorruption);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "World Corruption")
	void ReduceRegionCorruption(int32 RegionId, float ReductionAmount);

	// Test maps and scripted sequences can freeze passive growth without reinitializing replicated values.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "World Corruption")
	void SetPassiveIncreasePaused(bool bPaused);

	UFUNCTION(BlueprintPure, Category = "World Corruption")
	bool IsPassiveIncreasePaused() const { return bPassiveIncreasePaused; }

	UPROPERTY(BlueprintAssignable, Category = "World Corruption")
	FGPOnWorldCorruptionChanged OnWorldCorruptionChanged;

	UPROPERTY(BlueprintAssignable, Category = "World Corruption")
	FGPOnRegionCorruptionChanged OnRegionCorruptionChanged;

private:
	UPROPERTY(ReplicatedUsing = OnRep_WorldCorruption)
	float WorldCorruption = 0.0f;

	UPROPERTY(ReplicatedUsing = OnRep_RegionCorruptions)
	TArray<float> RegionCorruptions;

	// Replicate the clamp ceiling so every client normalizes presentation identically.
	UPROPERTY(ReplicatedUsing = OnRep_MaximumCorruption)
	float MaximumCorruption = 100.0f;

	TArray<float> LocalRegionCorruptionsShadow;
	float PassiveIncreasePerMinute = 0.0f;
	float PassiveTickInterval = 5.0f;
	bool bPassiveIncreaseConfigured = false;
	bool bPassiveIncreasePaused = false;
	FTimerHandle PassiveIncreaseTimerHandle;

	UFUNCTION()
	void OnRep_WorldCorruption();

	UFUNCTION()
	void OnRep_RegionCorruptions();

	UFUNCTION()
	void OnRep_MaximumCorruption();

	void HandlePassiveIncreaseTick();
	void RecalculateWorldCorruption();
	void BroadcastWorldCorruption();
	void BroadcastRegionCorruption(int32 RegionId);
	void StartPassiveIncrease();
	void StopPassiveIncrease();
	bool HasServerAuthority() const;
	float ClampCorruption(float Value) const;
};
