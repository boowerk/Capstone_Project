#pragma once

#include "CoreMinimal.h"
#include "Game/RegionEvents/GP_RegionEventActor.h"
#include "GP_StructureDefenseRegionEventActor.generated.h"

class UStaticMeshComponent;

/** Region event that asks players to survive around a defense structure for a fixed duration. */
UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_StructureDefenseRegionEventActor : public AGP_RegionEventActor
{
	GENERATED_BODY()

public:
	AGP_StructureDefenseRegionEventActor();

	virtual void ActivateRegionEvent() override;
	virtual void CompleteRegionEvent() override;
	virtual void ExpireRegionEvent() override;

	UFUNCTION(BlueprintPure, Category = "Region Event|Defense")
	float GetRemainingDefenseTime() const;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Region Event|Defense")
	TObjectPtr<UStaticMeshComponent> DefenseStructureMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Defense", meta = (ClampMin = "1.0"))
	float DefenseDurationSeconds = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Defense", meta = (ClampMin = "0.1"))
	float DefenseWaveIntervalSeconds = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Defense")
	bool bSpawnInitialDefenseWave = true;

private:
	FTimerHandle DefenseCompleteTimerHandle;
	FTimerHandle DefenseWaveTimerHandle;
	float DefenseEndWorldTimeSeconds = 0.0f;

	void SpawnDefenseWave();
	void StopDefenseTimers();
};
