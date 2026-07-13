#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Game/RegionEvents/GP_RegionEventData.h"
#include "Game/RegionEvents/GP_RegionEventActor.h"
#include "GP_RegionEventDirector.generated.h"

class AGP_EnemyCharacter;
class AGP_EnemySpawnVolume;
class APawn;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGPOnRegionEventStarted, AGP_RegionEventDirector*, Director, AGP_RegionEventActor*, EventActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGPOnRegionEventEnded, AGP_RegionEventDirector*, Director, AGP_RegionEventActor*, EventActor);

/**
 * Server-authoritative region event coordinator.
 * GameMode asks this actor to roll one event for a zone; the spawned event actor owns local presentation.
 */
UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_RegionEventDirector : public AActor
{
	GENERATED_BODY()

public:
	AGP_RegionEventDirector();

	UFUNCTION(BlueprintCallable, Category = "Region Event")
	void InitializeRegionEventDirector(int32 InRegionCount);

	UFUNCTION(BlueprintCallable, Category = "Region Event")
	AGP_RegionEventActor* TryStartRegionEventForZone(AGP_EnemySpawnVolume* Zone, EGPRegionEventTrigger Trigger);

	// Open-world maps can start an event from a region seed without requiring a linear enemy-spawn zone.
	UFUNCTION(BlueprintCallable, Category = "Region Event")
	AGP_RegionEventActor* TryStartRegionEventAtLocation(
		int32 RegionId,
		FVector SpawnLocation,
		EGPRegionEventTrigger Trigger = EGPRegionEventTrigger::ZoneStarted);

	UFUNCTION(BlueprintCallable, Category = "Region Event")
	void CompleteEventsForRegion(int32 RegionId);

	UFUNCTION(BlueprintCallable, Category = "Region Event")
	void CompleteEventsForZone(AGP_EnemySpawnVolume* Zone);

	UFUNCTION(BlueprintPure, Category = "Region Event")
	bool HasActiveEventForRegion(int32 RegionId) const;

	// Linear zones use this query to wait for explicit event objectives instead of ending after the first wave.
	UFUNCTION(BlueprintPure, Category = "Region Event")
	bool HasBlockingEventForRegion(int32 RegionId) const;

	UFUNCTION(BlueprintPure, Category = "Region Event")
	bool HasBlockingEventForZone(const AGP_EnemySpawnVolume* Zone) const;

	UFUNCTION(BlueprintPure, Category = "Region Event")
	int32 GetActiveEventCount() const { return ActiveEvents.Num(); }

	UFUNCTION(BlueprintPure, Category = "Region Event|Exploration")
	int32 GetExplorationEventPoolCount() const { return ExplorationEventPool.Num(); }

	UPROPERTY(BlueprintAssignable, Category = "Region Event")
	FGPOnRegionEventStarted OnRegionEventStarted;

	UPROPERTY(BlueprintAssignable, Category = "Region Event")
	FGPOnRegionEventEnded OnRegionEventEnded;

	UPROPERTY(BlueprintAssignable, Category = "Region Event")
	FGPOnRegionEventEnemySpawned OnRegionEventEnemySpawned;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event")
	bool bEnableRegionEvents = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event")
	bool bAutoActivateSpawnedEvents = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event", meta = (ClampMin = "0"))
	int32 MaxActiveEvents = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event")
	int32 RandomSeed = 4207;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event")
	TArray<TObjectPtr<UGP_RegionEventData>> EventPool;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event")
	TSubclassOf<AGP_RegionEventActor> RegionEventActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Placement")
	FVector SpawnOffset = FVector(0.0f, 0.0f, 60.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Cleanup", meta = (ClampMin = "0.0"))
	float FinishedEventLifeSpan = 4.0f;

	// Exploration events are evaluated independently from the legacy city-zone flow.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Exploration")
	bool bEnableExplorationEvents = true;

	// These production assets are separate from the user-authored map EventPool and remain BP-editable.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Exploration")
	TArray<TObjectPtr<UGP_RegionEventData>> ExplorationEventPool;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Exploration", meta = (ClampMin = "0.0", Units = "s"))
	float InitialExplorationDelaySeconds = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Exploration", meta = (ClampMin = "0.5", Units = "s"))
	float ExplorationEvaluationIntervalSeconds = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Exploration", meta = (ClampMin = "0.0", Units = "s"))
	float RegionDwellSeconds = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Exploration", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BaseExplorationChance = 0.30f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Exploration", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FullCorruptionChanceBonus = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Exploration", meta = (ClampMin = "0.0", Units = "s"))
	float GlobalExplorationCooldownSeconds = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Exploration", meta = (ClampMin = "1"))
	int32 MaxActiveExplorationEvents = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Exploration|Placement", meta = (ClampMin = "100.0", Units = "cm"))
	float MinimumExplorationSpawnDistance = 1400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Exploration|Placement", meta = (ClampMin = "100.0", Units = "cm"))
	float MaximumExplorationSpawnDistance = 2200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Exploration|Placement")
	FVector ExplorationNavProjectionExtent = FVector(900.0f, 900.0f, 1800.0f);

	// False by default makes repeated runs feel different while RandomSeed remains available for deterministic tests/BPs.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Exploration")
	bool bUseDeterministicRandomSeed = false;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<AGP_RegionEventActor>> ActiveEvents;

	FRandomStream EventRandomStream;
	int32 RegionCount = 0;
	bool bDirectorInitialized = false;
	int32 ObservedRegionId = INDEX_NONE;
	double ObservedRegionSinceSeconds = 0.0;
	double NextExplorationAttemptSeconds = 0.0;
	double GlobalExplorationCooldownUntilSeconds = 0.0;
	FName LastExplorationEventId = NAME_None;
	FTimerHandle ExplorationEvaluationTimerHandle;
	TMap<int32, double> RegionCooldownUntilSeconds;
	TSet<TWeakObjectPtr<AGP_RegionEventActor>> ExplorationEvents;
	TSet<TWeakObjectPtr<AGP_RegionEventActor>> AppliedOutcomeEvents;

	int32 ResolveRegionIdForZone(const AGP_EnemySpawnVolume* Zone) const;
	FVector ResolveSpawnLocationForZone(AGP_EnemySpawnVolume* Zone) const;
	AGP_RegionEventActor* SpawnSelectedEvent(
		int32 RegionId,
		const FVector& SpawnLocation,
		const UGP_RegionEventData* SelectedEvent,
		bool bExplorationEvent);
	void StartExplorationScheduler();
	void StopExplorationScheduler();
	void EvaluateExplorationEvents();
	bool ResolvePartyRegion(int32& OutRegionId, APawn*& OutReferencePawn) const;
	bool ResolveExplorationSpawnLocation(
		const APawn& ReferencePawn,
		int32 RegionId,
		FVector& OutSpawnLocation);
	const UGP_RegionEventData* SelectExplorationEvent(float NormalizedCorruption);
	int32 CountActiveExplorationEvents() const;
	float GetRegionCorruptionNormalized(int32 RegionId) const;
	void ApplyEventCorruptionOutcome(AGP_RegionEventActor& EventActor);
	void RemoveInactiveEvents();

	UFUNCTION()
	void HandleRegionEventStateChanged(AGP_RegionEventActor* EventActor);

	UFUNCTION()
	void HandleRegionEventEnemySpawned(AGP_RegionEventActor* EventActor, AGP_EnemyCharacter* Enemy);
};
