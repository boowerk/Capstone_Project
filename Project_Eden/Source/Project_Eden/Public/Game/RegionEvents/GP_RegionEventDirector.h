#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Game/RegionEvents/GP_RegionEventActor.h"
#include "Game/RegionEvents/GP_RegionEventData.h"
#include "GP_RegionEventDirector.generated.h"

class AGP_EnemyCharacter;
class AGP_RegionEventDirector;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FGPOnRegionEventStarted,
	AGP_RegionEventDirector*,
	Director,
	AGP_RegionEventActor*,
	EventActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FGPOnRegionEventEnded,
	AGP_RegionEventDirector*,
	Director,
	AGP_RegionEventActor*,
	EventActor);

/**
 * Server-authoritative coordinator for deterministic graduation-demo encounters.
 * It resolves an explicit EventId only; ambient timers, weighted rolls, and random placement are intentionally absent.
 */
UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_RegionEventDirector : public AActor
{
	GENERATED_BODY()

public:
	AGP_RegionEventDirector();

	UFUNCTION(BlueprintCallable, Category = "Demo Event")
	void InitializeRegionEventDirector(int32 InRegionCount);

	UFUNCTION(
		BlueprintCallable,
		BlueprintAuthorityOnly,
		Category = "Demo Event",
		meta = (AdvancedDisplay = "DormantWaitTimeoutSeconds,RequiredApproachPlayers"))
	AGP_RegionEventActor* TryStartGuidedRegionEventAtLocation(
		FName EventId,
		int32 RegionId,
		FVector SpawnLocation,
		float DormantWaitTimeoutSeconds = -1.0f,
		int32 RequiredApproachPlayers = 2);

	UFUNCTION(BlueprintCallable, Category = "Demo Event")
	void CompleteEventsForRegion(int32 RegionId);

	UFUNCTION(BlueprintPure, Category = "Demo Event")
	bool HasActiveEventForRegion(int32 RegionId) const;

	UFUNCTION(BlueprintPure, Category = "Demo Event")
	int32 GetActiveEventCount() const;

	UFUNCTION(BlueprintPure, Category = "Demo Event")
	int32 GetGuidedEventPoolCount() const { return GuidedEventPool.Num(); }

	UPROPERTY(BlueprintAssignable, Category = "Demo Event")
	FGPOnRegionEventStarted OnRegionEventStarted;

	UPROPERTY(BlueprintAssignable, Category = "Demo Event")
	FGPOnRegionEventEnded OnRegionEventEnded;

	UPROPERTY(BlueprintAssignable, Category = "Demo Event")
	FGPOnRegionEventEnemySpawned OnRegionEventEnemySpawned;

protected:
	virtual void BeginPlay() override;

	// This switch controls only explicitly requested scripted beats; it cannot enable random encounters.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Demo Event")
	bool bEnableScriptedEvents = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Demo Event", meta = (ClampMin = "1"))
	int32 MaxActiveEvents = 1;

	// The fixed route resolves these definitions by unique EventId.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Demo Event")
	TArray<TObjectPtr<UGP_RegionEventData>> GuidedEventPool;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Demo Event")
	TSubclassOf<AGP_RegionEventActor> RegionEventActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Demo Event|Placement")
	FVector SpawnOffset = FVector(0.0f, 0.0f, 60.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Demo Event|Cleanup", meta = (ClampMin = "0.0"))
	float FinishedEventLifeSpan = 4.0f;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<AGP_RegionEventActor>> ActiveEvents;

	int32 RegionCount = 0;
	bool bDirectorInitialized = false;

	const UGP_RegionEventData* FindUniqueGuidedEventById(FName EventId) const;
	AGP_RegionEventActor* SpawnGuidedEvent(
		int32 RegionId,
		const FVector& SpawnLocation,
		const UGP_RegionEventData& EventData,
		float DormantWaitTimeoutSeconds,
		int32 RequiredApproachPlayers);
	void RemoveInactiveEvents();

	UFUNCTION()
	void HandleRegionEventStateChanged(AGP_RegionEventActor* EventActor);

	UFUNCTION()
	void HandleRegionEventEnemySpawned(AGP_RegionEventActor* EventActor, AGP_EnemyCharacter* Enemy);

#if WITH_DEV_AUTOMATION_TESTS
	friend class FGPRegionEventGuidedDirectorControlTest;
#endif
};
