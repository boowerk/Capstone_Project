#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GP_GameMode.generated.h"

class AGP_EnemyCharacter;
class AGP_EnemySpawnVolume;
class AGP_GameState;

/**
 * Server-authoritative progression manager for the linear "city -> boss room -> next city" loop.
 *
 * Zones are defined entirely by AGP_EnemySpawnVolume actors placed in the level: this mode collects them,
 * orders them by ZoneOrder, and for each zone spawns its enemy composition on the navmesh, tracks deaths,
 * and advances when the zone is cleared. Designers only place volumes and fill their Spawns; no Blueprint
 * spawn wiring is required.
 *
 * Blueprint hooks remain for presentation: OnZoneStarted (extra setup after spawn), OnZoneCompleted
 * (cleared-city vegetation change + boss-room teleport/gate), OnRunFinished (win/lose UI).
 * See the PCG/gameplay-flow topic note.
 */
UCLASS()
class PROJECT_EDEN_API AGP_GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGP_GameMode();

	// Begins the run (spawns zone 0). Called automatically after a short delay; exposed so Blueprints can
	// trigger it explicitly once the level/navmesh is ready.
	UFUNCTION(BlueprintCallable, Category = "Run")
	void StartRun();

	// Call when all players are dead/downed to end the run as a loss. Player-death detection lives outside this class.
	UFUNCTION(BlueprintCallable, Category = "Run")
	void NotifyAllPlayersDead();

protected:
	virtual void BeginPlay() override;

	// Fired after a zone's enemies have been spawned, for extra presentation/setup. Spawning itself is automatic.
	UFUNCTION(BlueprintImplementableEvent, Category = "Run|Zone")
	void OnZoneStarted(int32 ZoneIndex, AGP_EnemySpawnVolume* Zone);

	// All enemies in the zone are dead. Wire the cleared-city vegetation change and the boss-room teleport/gate here.
	UFUNCTION(BlueprintImplementableEvent, Category = "Run|Zone")
	void OnZoneCompleted(int32 ZoneIndex, AGP_EnemySpawnVolume* Zone);

	// The whole run is finished. bVictory distinguishes clearing the last zone from a wipe.
	UFUNCTION(BlueprintImplementableEvent, Category = "Run")
	void OnRunFinished(bool bVictory);

	// Delay before auto-starting the run, giving navmesh/streaming time to be ready before the first spawn.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run", meta = (ClampMin = "0.0", Units = "s"))
	float StartDelaySeconds = 1.0f;

private:
	// Spawn-volume zones discovered in the level, ascending by ZoneOrder. Built in BeginPlay.
	UPROPERTY(Transient)
	TArray<TObjectPtr<AGP_EnemySpawnVolume>> OrderedZones;

	int32 CurrentZoneIndex = INDEX_NONE;
	int32 AliveZoneEnemies = 0;
	bool bRunStarted = false;
	bool bRunFinished = false;

	void GatherZones();
	void StartZone(int32 ZoneIndex);
	void SpawnZoneEnemies(AGP_EnemySpawnVolume* Zone);
	void RegisterZoneEnemy(AGP_EnemyCharacter* Enemy);
	void CompleteCurrentZone();
	void AdvanceZone();
	void FinishRun(bool bVictory);

	UFUNCTION()
	void HandleZoneEnemyDied(AGP_EnemyCharacter* DeadEnemy);

	AGP_GameState* GetGPGameState() const;
};
