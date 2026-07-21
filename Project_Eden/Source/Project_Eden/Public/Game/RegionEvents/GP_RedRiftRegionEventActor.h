#pragma once

#include "CoreMinimal.h"
#include "Game/RegionEvents/GP_RegionEventActor.h"
#include "GP_RedRiftRegionEventActor.generated.h"

/** Region event that periodically calls monster waves from a corrupted red rift. */
UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_RedRiftRegionEventActor : public AGP_RegionEventActor
{
	GENERATED_BODY()

public:
	virtual void ActivateRegionEvent() override;
	virtual void CompleteRegionEvent() override;
	virtual void ExpireRegionEvent() override;

	// Pure policy is shared by runtime completion and automation tests, including the infinite-wave edge case.
	static bool ShouldCompleteAfterWaveSequence(int32 InMaxWaveCount, int32 InSpawnedWaveCount, int32 InAliveEnemyCount);

	UFUNCTION(BlueprintPure, Category = "Region Event|Red Rift")
	int32 GetSpawnedWaveCount() const { return SpawnedWaveCount; }

	UFUNCTION(BlueprintPure, Category = "Region Event|Red Rift")
	bool HaveAllConfiguredWavesSpawned() const { return bAllConfiguredWavesSpawned; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnTrackedEnemyCountChanged() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Red Rift", meta = (ClampMin = "0.0"))
	float InitialWaveDelaySeconds = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Red Rift", meta = (ClampMin = "0.1"))
	float WaveIntervalSeconds = 8.0f;

	// <= 0 means keep spawning until the event duration/zone clear ends.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Red Rift")
	int32 MaxWaveCount = 3;

private:
	FTimerHandle WaveTimerHandle;
	int32 SpawnedWaveCount = 0;
	bool bAllConfiguredWavesSpawned = false;

	void SpawnWave();
	void StopWaveTimer();
	void MarkAllConfiguredWavesSpawned();
	void TryCompleteAfterFinalWave();
};
