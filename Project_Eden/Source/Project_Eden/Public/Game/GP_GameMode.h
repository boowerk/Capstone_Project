#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GP_GameMode.generated.h"

class AGP_EnemyCharacter;
class AGP_EnemySpawnVolume;
class AGP_GameState;
class AGP_RunPortal;
class AGP_EnemySpawnMarker;
class AGP_RegionEventActor;
class AGP_RegionEventDirector;
enum class EGPRegionEventTrigger : uint8;

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

	// Portal actor spawned at a cleared zone to teleport players into the next zone. Set a BP child
	// (with mesh/VFX) here. If left empty, zones simply unlock and players walk in.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run")
	TSubclassOf<AGP_RunPortal> PortalClass;

	// Map to send the party back to once a run ends (victory or defeat). The
	// delay lets clients see the result screen before the lobby travel kicks in.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run")
	FString ReturnMapName = TEXT("MainMap/LobbyMap");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run", meta = (ClampMin = "0.0"))
	float ReturnToLobbyDelay = 8.0f;

	// Region revival config. In the current RegionStateManager/PCG setup,
	// state 0 is the healthy/alive vegetation state.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Region", meta = (ClampMin = "0"))
	int32 RegionCount = 9;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Region")
	uint8 DeadRegionState = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Region")
	uint8 AliveRegionState = 0;

	// Corruption progression is initialized together with the existing region state array.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Corruption")
	bool bEnableWorldCorruption = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Corruption", meta = (EditCondition = "bEnableWorldCorruption", ClampMin = "0.0"))
	float InitialCorruption = 15.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Corruption", meta = (EditCondition = "bEnableWorldCorruption", ClampMin = "1.0"))
	float MaximumCorruption = 100.0f;

	// The default adds two corruption points per real-time minute to every region.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Corruption", meta = (EditCondition = "bEnableWorldCorruption", ClampMin = "0.0"))
	float PassiveCorruptionIncreasePerMinute = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Corruption", meta = (EditCondition = "bEnableWorldCorruption", ClampMin = "0.1", Units = "s"))
	float CorruptionUpdateInterval = 5.0f;

	// Optional regional event coordinator. Place one in the map for authored pools, or provide a class to auto-spawn.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Region Events")
	TSubclassOf<AGP_RegionEventDirector> RegionEventDirectorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Region Events")
	bool bAutoSpawnRegionEventDirector = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Region Events")
	bool bStartRegionEventsOnZoneStart = true;

	// Completion events are meant for reward/cleanup presentation. Enemy-spawning completion events should stay disabled.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Region Events")
	bool bStartRegionEventsOnZoneCompleted = false;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<AGP_EnemySpawnVolume>> OrderedZones;

	UPROPERTY(Transient)
	TObjectPtr<AGP_RegionEventDirector> RegionEventDirector;

	int32 CurrentZoneIndex = INDEX_NONE;
	int32 PendingZoneIndex = INDEX_NONE;
	int32 AliveZoneEnemies = 0;
	int32 MarkersTotal = 0;
	int32 MarkersTriggered = 0;
	bool bRunStarted = false;
	bool bRunFinished = false;

	FTimerHandle ReturnToLobbyTimerHandle;

	void GatherZones();
	void ResolveRegionEventDirector();
	void InitializeRegionStates();
	void UnlockZone(int32 ZoneIndex);
	void StartZone(int32 ZoneIndex);
	void StartRegionEventForZone(AGP_EnemySpawnVolume* Zone, EGPRegionEventTrigger Trigger);
	void SpawnZoneEnemies(AGP_EnemySpawnVolume* Zone);
	void SpawnMarkerEnemies(AGP_EnemySpawnVolume* Zone, AGP_EnemySpawnMarker* Marker);
	void RegisterZoneEnemy(AGP_EnemyCharacter* Enemy, int32 CorruptionRegionId = INDEX_NONE);
	void MaybeCompleteZone();
	void CompleteCurrentZone();
	void AdvanceZone();
	void SpawnPortalToZone(int32 FromZoneIndex, int32 ToZoneIndex);
	void FinishRun(bool bVictory);
	void ReturnToLobby();

	UFUNCTION()
	void HandlePlayerEnteredZone(AGP_EnemySpawnVolume* Zone);

	UFUNCTION()
	void HandleMarkerTriggered(AGP_EnemySpawnVolume* Zone, AGP_EnemySpawnMarker* Marker);

	UFUNCTION()
	void HandleZoneEnemyDied(AGP_EnemyCharacter* DeadEnemy);

	UFUNCTION()
	void HandleRegionEventEnemySpawned(AGP_RegionEventActor* EventActor, AGP_EnemyCharacter* Enemy);

	AGP_GameState* GetGPGameState() const;
};
