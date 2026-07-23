#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GP_GameMode.generated.h"

class AGP_EnemyCharacter;
class AGP_CorruptionPresentationActor;
class AGP_EnemySpawnVolume;
class AGP_GameState;
class AGP_RunPortal;
class AGP_EnemySpawnMarker;
class AGP_RegionEventActor;
class AGP_RegionEventDirector;
class AGP_VillageLayoutDirector;
class APlayerState;
enum class EGPRegionEventTrigger : uint8;
enum class EGPZoneStage : uint8;

struct FGPZoneRuntimeState
{
	TWeakObjectPtr<AGP_EnemySpawnVolume> Zone;
	TSet<TWeakObjectPtr<APlayerState>> Participants;
	TSet<TWeakObjectPtr<APlayerState>> PresentPlayers;
	int32 MarkersTotal = 0;
	int32 MarkersTriggered = 0;
	int32 AliveEnemies = 0;
	FVector LastEnemyDeathLocation = FVector::ZeroVector;
	bool bHasLastEnemyDeathLocation = false;
	bool bStarted = false;
	bool bCompleted = false;
};

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

	UFUNCTION(BlueprintPure, Category = "Run")
	int32 GetRunSeed() const { return RunSeed; }

	// Manual trigger — bypasses overlap and immediately spawns zone 0. Use from BP if needed.
	UFUNCTION(BlueprintCallable, Category = "Run")
	void StartRun();

	UFUNCTION(BlueprintCallable, Category = "Run")
	void NotifyAllPlayersDead();

protected:
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void InitGameState() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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
	int32 RegionCount = 15;

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

	// The replicated native actor works out of the box; assign a BP child to customize skybox material presentation.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Corruption|Presentation")
	bool bAutoSpawnCorruptionPresentation = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Corruption|Presentation", meta = (EditCondition = "bAutoSpawnCorruptionPresentation"))
	TSubclassOf<AGP_CorruptionPresentationActor> CorruptionPresentationClass;

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

	// Optional run-layout coordinator. The native default only selects VillageSlot candidates;
	// it does not spawn cities or change gameplay zones.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Village Layout")
	TSubclassOf<AGP_VillageLayoutDirector> VillageLayoutDirectorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Village Layout")
	bool bAutoSpawnVillageLayoutDirector = true;

	// Active gameplay zones act as navigation invokers, keeping runtime NavMesh generation
	// around unlocked villages instead of around every player or across the entire landscape.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Navigation")
	bool bUseZoneNavigationInvokers = true;

	// 180 m covers the corners of the current 230 m medium-village footprint.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Navigation",
		meta = (EditCondition = "bUseZoneNavigationInvokers", ClampMin = "1000.0", Units = "cm"))
	float ZoneNavigationGenerationRadius = 18000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Navigation",
		meta = (EditCondition = "bUseZoneNavigationInvokers", ClampMin = "1000.0", Units = "cm"))
	float ZoneNavigationRemovalRadius = 22000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Navigation",
		meta = (EditCondition = "bUseZoneNavigationInvokers", ClampMin = "0.05", Units = "s"))
	float ZoneNavigationRetryInterval = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Navigation",
		meta = (EditCondition = "bUseZoneNavigationInvokers", ClampMin = "1.0", Units = "s"))
	float ZoneNavigationReadyTimeout = 10.0f;

private:
	int32 RunSeed = INDEX_NONE;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AGP_EnemySpawnVolume>> OrderedZones;

	UPROPERTY(Transient)
	TObjectPtr<AGP_RegionEventDirector> RegionEventDirector;

	UPROPERTY(Transient)
	TObjectPtr<AGP_VillageLayoutDirector> VillageLayoutDirector;

	TMap<TWeakObjectPtr<AGP_EnemySpawnVolume>, FGPZoneRuntimeState> ZoneRuntimeStates;
	TMap<TWeakObjectPtr<AGP_EnemyCharacter>, TWeakObjectPtr<AGP_EnemySpawnVolume>> EnemyOwningZones;
	TMap<TWeakObjectPtr<AGP_EnemySpawnVolume>, int32> ZoneNavigationStartRetries;
	TSet<EGPZoneStage> UnlockedStages;
	TSet<TWeakObjectPtr<AGP_EnemySpawnVolume>> RegisteredNavigationInvokerZones;
	bool bUseStagedZoneProgression = false;
	bool bZoneProgressionInitialized = false;

	int32 CurrentZoneIndex = INDEX_NONE;
	int32 PendingZoneIndex = INDEX_NONE;
	int32 AliveZoneEnemies = 0;

	// Location of the most recent zone enemy death. The advance portal spawns here
	// (where the last monster fell) instead of the zone center. Reset per zone start.
	FVector LastEnemyDeathLocation = FVector::ZeroVector;
	bool bHasLastDeathLocation = false;
	int32 MarkersTotal = 0;
	int32 MarkersTriggered = 0;
	bool bRunStarted = false;
	bool bRunFinished = false;

	FTimerHandle ReturnToLobbyTimerHandle;

	void GatherZones();
	void InitializeZoneProgression();
	void BindZoneDelegates(AGP_EnemySpawnVolume* Zone);
	void ResolveVillageLayoutDirector();
	void ResolveRegionEventDirector();
	void InitializeRegionStates();
	void SpawnCorruptionPresentation();
	void RegisterZoneNavigationInvoker(AGP_EnemySpawnVolume* Zone);
	void UnregisterZoneNavigationInvoker(AGP_EnemySpawnVolume* Zone);
	void UnregisterAllZoneNavigationInvokers();
	void UnlockZone(int32 ZoneIndex);
	void StartZone(int32 ZoneIndex);
	void StartRegionEventForZone(AGP_EnemySpawnVolume* Zone, EGPRegionEventTrigger Trigger);
	void SpawnZoneEnemies(AGP_EnemySpawnVolume* Zone);
	void SpawnMarkerEnemies(AGP_EnemySpawnVolume* Zone, AGP_EnemySpawnMarker* Marker);
	void RegisterZoneEnemy(
		AGP_EnemyCharacter* Enemy,
		AGP_EnemySpawnVolume* OwningZone,
		int32 CorruptionRegionId = INDEX_NONE);
	void MaybeCompleteZone();
	void CompleteCurrentZone();
	void AdvanceZone();
	void SpawnPortalToZone(int32 FromZoneIndex, int32 ToZoneIndex);
	void FinishRun(bool bVictory);
	void ReturnToLobby();
	void UnlockStage(EGPZoneStage Stage);
	void StartStagedZone(AGP_EnemySpawnVolume* Zone);
	void MaybeCompleteStagedZone(AGP_EnemySpawnVolume* Zone);
	void CompleteStagedZone(AGP_EnemySpawnVolume* Zone);
	void EvaluateStagedProgression();
	bool AreAllZonesInStageCompleted(EGPZoneStage Stage) const;
	bool HaveAllActivePlayersMiddleCredit() const;
	bool AreAllActivePlayersPresent(const FGPZoneRuntimeState& State) const;
	int32 GetTotalAliveZoneEnemies() const;
	AGP_EnemySpawnVolume* FindActiveZoneForRegion(int32 RegionId) const;
	AGP_EnemySpawnVolume* FindNearestZoneInStage(
		const AGP_EnemySpawnVolume* FromZone,
		EGPZoneStage Stage) const;
	void SpawnPortalBetweenZones(
		AGP_EnemySpawnVolume* FromZone,
		AGP_EnemySpawnVolume* ToZone,
		const FVector& PreferredSpawnLocation,
		int32 RetryCount = 0);
	void AssignPlayersToOuterVillageStarts();

	UFUNCTION()
	void HandlePlayerEnteredZone(AGP_EnemySpawnVolume* Zone);

	UFUNCTION()
	void HandlePlayerEnteredZoneDetailed(AGP_EnemySpawnVolume* Zone, APlayerState* PlayerState);

	UFUNCTION()
	void HandlePlayerExitedZoneDetailed(AGP_EnemySpawnVolume* Zone, APlayerState* PlayerState);

	UFUNCTION()
	void HandleVillageRuntimeLayoutReady();

	UFUNCTION()
	void HandleMarkerTriggered(AGP_EnemySpawnVolume* Zone, AGP_EnemySpawnMarker* Marker);

	UFUNCTION()
	void HandleZoneEnemyDied(AGP_EnemyCharacter* DeadEnemy);

	UFUNCTION()
	void HandleRegionEventEnemySpawned(AGP_RegionEventActor* EventActor, AGP_EnemyCharacter* Enemy);

	AGP_GameState* GetGPGameState() const;
};
