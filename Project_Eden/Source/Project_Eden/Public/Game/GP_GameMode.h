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
 * Zones are AGP_EnemySpawnVolume actors placed in the level. GameMode collects and sorts them by
 * ZoneOrder. Zone 0 is unlocked at BeginPlay; enemies spawn when the player physically enters the box.
 * On clear, the next zone unlocks and waits for player entry.
 */
UCLASS()
class PROJECT_EDEN_API AGP_GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGP_GameMode();

	// Manual trigger — bypasses overlap and immediately spawns zone 0. Use from BP if needed.
	UFUNCTION(BlueprintCallable, Category = "Run")
	void StartRun();

	UFUNCTION(BlueprintCallable, Category = "Run")
	void NotifyAllPlayersDead();

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Run|Zone")
	void OnZoneStarted(int32 ZoneIndex, AGP_EnemySpawnVolume* Zone);

	UFUNCTION(BlueprintImplementableEvent, Category = "Run|Zone")
	void OnZoneCompleted(int32 ZoneIndex, AGP_EnemySpawnVolume* Zone);

	UFUNCTION(BlueprintImplementableEvent, Category = "Run")
	void OnRunFinished(bool bVictory);

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<AGP_EnemySpawnVolume>> OrderedZones;

	int32 CurrentZoneIndex = INDEX_NONE;
	int32 PendingZoneIndex = INDEX_NONE;
	int32 AliveZoneEnemies = 0;
	bool bRunStarted = false;
	bool bRunFinished = false;

	void GatherZones();
	void UnlockZone(int32 ZoneIndex);
	void StartZone(int32 ZoneIndex);
	void SpawnZoneEnemies(AGP_EnemySpawnVolume* Zone);
	void RegisterZoneEnemy(AGP_EnemyCharacter* Enemy);
	void CompleteCurrentZone();
	void AdvanceZone();
	void FinishRun(bool bVictory);

	UFUNCTION()
	void HandlePlayerEnteredZone(AGP_EnemySpawnVolume* Zone);

	UFUNCTION()
	void HandleZoneEnemyDied(AGP_EnemyCharacter* DeadEnemy);

	AGP_GameState* GetGPGameState() const;
};
