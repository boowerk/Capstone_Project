#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP_EnemySpawnVolume.generated.h"

class AGP_EnemyCharacter;
class UBoxComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGPOnPlayerEnteredZone, AGP_EnemySpawnVolume*, Zone);

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
 * Enemies spawn only when a player physically enters the box, not at game start.
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

	// Called by GameMode to allow this zone to fire OnPlayerEnteredZone when a player steps in.
	void Unlock();

	// Returns a spawn location projected onto the navmesh. Random inside the box when bRandomizeInVolume is
	// true (city enemies); the box center otherwise (precise boss placement). bOutProjected reports whether
	// a navmesh point was found.
	FVector GetSpawnPoint(bool bRandomizeInVolume, bool& bOutProjected) const;

	// Fired once when the first player enters this zone after Unlock() is called.
	UPROPERTY(BlueprintAssignable, Category = "Zone")
	FGPOnPlayerEnteredZone OnPlayerEnteredZone;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	TArray<FGP_EnemySpawnEntry> Spawns;

private:
	bool bUnlocked = false;
	bool bTriggered = false;

	UFUNCTION()
	void HandleOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
