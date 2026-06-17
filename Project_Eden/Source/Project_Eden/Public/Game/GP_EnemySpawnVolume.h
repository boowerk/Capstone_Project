#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP_EnemySpawnVolume.generated.h"

class AGP_EnemyCharacter;
class UBoxComponent;

/** One enemy type and how many of it spawn in this zone. */
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
 * Placed-in-level actor that fully defines one progression zone: a box area, its order in the run,
 * whether it is the boss zone, and which enemies spawn inside it. AGP_GameMode discovers all of these,
 * orders them by ZoneOrder, and drives the linear "city -> boss room -> next city" loop.
 *
 * Design intent: a designer drops one of these per city (and per boss room), sizes the box over the
 * combat area, and fills Spawns. No Blueprint spawn wiring is needed. See the PCG/gameplay-flow topic note.
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
	const TArray<FGP_EnemySpawnEntry>& GetSpawns() const { return Spawns; }

	// Returns a spawn location projected onto the navmesh. Random inside the box when bRandomizeInVolume is
	// true (city enemies); the box center otherwise (precise boss placement). bOutProjected reports whether
	// a navmesh point was found.
	FVector GetSpawnPoint(bool bRandomizeInVolume, bool& bOutProjected) const;

protected:
	// The combat area for this zone; enemies spawn within its bounds.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zone")
	TObjectPtr<UBoxComponent> SpawnBox;

	// Ascending run order: 0 is the first zone, 1 the next, etc. Determines the city -> boss -> city sequence.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	int32 ZoneOrder = 0;

	// Shown in the HUD ("City of X", "Boss: Crystal Seraph").
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	FText DisplayName;

	// Boss zones report the BossFight phase and spawn at the box center (precise) instead of randomized.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	bool bIsBossZone = false;

	// Enemy composition spawned when this zone starts.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	TArray<FGP_EnemySpawnEntry> Spawns;
};
