#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP_EnemySpawnVolume.generated.h"

class AGP_EnemyCharacter;
class AGP_EnemySpawnMarker;
class UBoxComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGPOnPlayerEnteredZone, AGP_EnemySpawnVolume*, Zone);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGPOnMarkerTriggered, AGP_EnemySpawnVolume*, Zone, AGP_EnemySpawnMarker*, Marker);

/** One enemy type and how many of it spawn. */
USTRUCT(BlueprintType)
struct FGP_EnemySpawnEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	TSubclassOf<AGP_EnemyCharacter> EnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (ClampMin = "1"))
	int32 Count = 1;
};

/**
 * Placed-in-level actor defining one progression zone (a city or a boss room).
 *
 * Two spawn modes:
 *  - Marker mode: place AGP_EnemySpawnMarker actors inside the box. This zone collects them at
 *    BeginPlay. Enemies spawn marker-by-marker as the player advances into the city.
 *    Zone clears only when every marker has triggered and all enemies are dead.
 *  - Box-fallback mode: if no markers exist, the flat Spawns array spawns all at once on zone entry
 *    (boss zones always use this mode — center spawn, no markers needed).
 */
UCLASS()
class PROJECT_EDEN_API AGP_EnemySpawnVolume : public AActor
{
	GENERATED_BODY()

public:
	AGP_EnemySpawnVolume();

	int32 GetZoneOrder() const { return ZoneOrder; }
	bool IsBossZone() const { return bIsBossZone; }
	FText GetDisplayName() const { return DisplayName; }
	const TArray<int32>& GetRegionsToRevive() const { return RegionsToRevive; }
	int32 GetCorruptionRegionId() const;
	const TArray<FGP_EnemySpawnEntry>& GetSpawns() const { return Spawns; }
	const TArray<TObjectPtr<AGP_EnemySpawnMarker>>& GetMarkers() const { return Markers; }
	int32 GetMarkerCount() const { return Markers.Num(); }

	void Unlock();
	void ActivateMarkers();

	// Called by AGP_EnemySpawnMarker when a player enters its radius.
	void NotifyMarkerTriggered(AGP_EnemySpawnMarker* Marker);

	FVector GetSpawnPoint(bool bRandomizeInVolume, bool& bOutProjected) const;
	FVector GetSpawnPointNearMarker(const AGP_EnemySpawnMarker* Marker, bool& bOutProjected) const;

	UPROPERTY(BlueprintAssignable, Category = "Zone")
	FGPOnPlayerEnteredZone OnPlayerEnteredZone;

	UPROPERTY(BlueprintAssignable, Category = "Zone")
	FGPOnMarkerTriggered OnMarkerTriggered;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zone")
	TObjectPtr<UBoxComponent> SpawnBox;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	int32 ZoneOrder = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	bool bIsBossZone = false;

	// Region ids that revive (turn alive) when this zone is cleared. Lets a city
	// clear light up its surrounding nature regions. Empty = revives nothing.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone|Region")
	TArray<int32> RegionsToRevive;

	// -1 automatically uses the first revived region, then ZoneOrder as a final fallback.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone|Corruption", meta = (ClampMin = "-1"))
	int32 CorruptionRegionId = INDEX_NONE;

	// Box-fallback composition (used when no markers exist, or for boss zones).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	TArray<FGP_EnemySpawnEntry> Spawns;

private:
	UPROPERTY()
	TArray<TObjectPtr<AGP_EnemySpawnMarker>> Markers;

	bool bUnlocked = false;
	bool bEntered = false;

	FVector ProjectToNavmesh(const FVector& DesiredLocation, bool& bOutProjected) const;

	UFUNCTION()
	void HandleBoxOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
