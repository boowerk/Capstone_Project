#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Game/RegionEvents/GP_RegionEventData.h"
#include "Game/RegionEvents/GP_RegionEventActor.h"
#include "GP_RegionEventDirector.generated.h"

class AGP_EnemyCharacter;
class AGP_EnemySpawnVolume;

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

	UFUNCTION(BlueprintCallable, Category = "Region Event")
	void CompleteEventsForRegion(int32 RegionId);

	UFUNCTION(BlueprintCallable, Category = "Region Event")
	void CompleteEventsForZone(AGP_EnemySpawnVolume* Zone);

	UFUNCTION(BlueprintPure, Category = "Region Event")
	bool HasActiveEventForRegion(int32 RegionId) const;

	UFUNCTION(BlueprintPure, Category = "Region Event")
	int32 GetActiveEventCount() const { return ActiveEvents.Num(); }

	UPROPERTY(BlueprintAssignable, Category = "Region Event")
	FGPOnRegionEventStarted OnRegionEventStarted;

	UPROPERTY(BlueprintAssignable, Category = "Region Event")
	FGPOnRegionEventEnded OnRegionEventEnded;

	UPROPERTY(BlueprintAssignable, Category = "Region Event")
	FGPOnRegionEventEnemySpawned OnRegionEventEnemySpawned;

protected:
	virtual void BeginPlay() override;

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

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<AGP_RegionEventActor>> ActiveEvents;

	FRandomStream EventRandomStream;
	int32 RegionCount = 0;
	bool bDirectorInitialized = false;

	int32 ResolveRegionIdForZone(const AGP_EnemySpawnVolume* Zone) const;
	FVector ResolveSpawnLocationForZone(AGP_EnemySpawnVolume* Zone) const;
	void RemoveInactiveEvents();

	UFUNCTION()
	void HandleRegionEventStateChanged(AGP_RegionEventActor* EventActor);

	UFUNCTION()
	void HandleRegionEventEnemySpawned(AGP_RegionEventActor* EventActor, AGP_EnemyCharacter* Enemy);
};
