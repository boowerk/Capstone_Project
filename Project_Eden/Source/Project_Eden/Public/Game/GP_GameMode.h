#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GP_GameMode.generated.h"

class AGP_EnemyCharacter;
class AGP_EnemySpawnVolume;
class AGP_GameState;
class AGP_RunPortal;
class AGP_EnemySpawnMarker;
class AGP_VillageLayoutDirector;
class AGP_PlayerController;
class APlayerController;
class APlayerState;
class APawn;
class APlayerStart;
enum class EGPZoneStage : uint8;

struct FGPZoneRuntimeState
{
	TWeakObjectPtr<AGP_EnemySpawnVolume> Zone;
	TSet<TWeakObjectPtr<APlayerState>> Participants;
	TSet<TWeakObjectPtr<APlayerState>> PresentPlayers;
	int32 MarkersTotal = 0;
	int32 MarkersTriggered = 0;
	int32 AliveEnemies = 0;
	bool bBossPhaseStarted = false;
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

	// Called by a Middle-selection portal on the server.
	void OpenMiddleTravelSelection(AGP_PlayerController* PlayerController);
	bool RequestMiddleTravel(
		AGP_PlayerController* PlayerController,
		FName DestinationZoneId);
	void NotifyVillageClientVisualReady(
		AGP_PlayerController* PlayerController,
		int32 LayoutRevision);
	virtual void RestartPlayer(AController* NewPlayer) override;

	// Pure progression policy: authored Outer slots are candidates, while only
	// the unique zones assigned to active players are required for advancement.
	static bool IsOuterStageReady(
		int32 ActivePlayerCount,
		int32 AssignedOuterZoneCount,
		int32 CompletedAssignedOuterZoneCount);

protected:
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void InitGameState() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual void Logout(AController* Exiting) override;

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

	// Any gameplay map can author one or more anchors. The server expands the available starts into
	// collision-checked slots so a three-player party never relies on spawn collision nudging.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Network|Spawn", meta = (ClampMin = "1"))
	int32 RequiredPartyPlayerStartCount = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Network|Spawn", meta = (ClampMin = "150.0", Units = "cm"))
	float PartyPlayerStartSpacing = 260.0f;
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

	// Navigation only needs the authored combat area, not the full 230 m village footprint.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Navigation",
		meta = (EditCondition = "bUseZoneNavigationInvokers", ClampMin = "1000.0", Units = "cm"))
	float ZoneNavigationGenerationRadius = 12000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Navigation",
		meta = (EditCondition = "bUseZoneNavigationInvokers", ClampMin = "1000.0", Units = "cm"))
	float ZoneNavigationRemovalRadius = 15000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Navigation",
		meta = (EditCondition = "bUseZoneNavigationInvokers", ClampMin = "0.05", Units = "s"))
	float ZoneNavigationRetryInterval = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Navigation",
		meta = (EditCondition = "bUseZoneNavigationInvokers", ClampMin = "1.0", Units = "s"))
	float ZoneNavigationReadyTimeout = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Middle Travel")
	FName VillagePortalAnchorTag = TEXT("VillagePortalAnchor");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Middle Travel")
	FName MiddleArrivalAnchorTag = TEXT("MiddleArrivalAnchor");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Middle Travel",
		meta = (ClampMin = "100.0", Units = "cm"))
	float MiddleTravelPortalUseRadius = 600.0f;

private:
	int32 RunSeed = INDEX_NONE;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AGP_EnemySpawnVolume>> OrderedZones;

	UPROPERTY(Transient)
	TArray<TObjectPtr<APlayerStart>> RuntimePartyPlayerStarts;

	// Weak slots also include map-authored starts; only spawned starts need a strong transient reference above.
	TArray<TWeakObjectPtr<APlayerStart>> PartyPlayerStartSlots;
	TMap<TWeakObjectPtr<AController>, int32> PartyStartSlotByController;

	// Populated from placed BP_RegionSeed actors when a map authors distinct biome states.
	TArray<uint8> RuntimeInitialRegionStates;

	UPROPERTY(Transient)
	TObjectPtr<AGP_VillageLayoutDirector> VillageLayoutDirector;

	TMap<TWeakObjectPtr<AGP_EnemySpawnVolume>, FGPZoneRuntimeState> ZoneRuntimeStates;
	TMap<TWeakObjectPtr<AGP_EnemyCharacter>, TWeakObjectPtr<AGP_EnemySpawnVolume>> EnemyOwningZones;
	TMap<TWeakObjectPtr<AGP_EnemySpawnVolume>, int32> ZoneNavigationStartRetries;
	TSet<EGPZoneStage> UnlockedStages;
	TSet<TWeakObjectPtr<AGP_EnemySpawnVolume>> RegisteredNavigationInvokerZones;
	TMap<TWeakObjectPtr<APlayerController>, int32> VillageVisualReadyRevisions;
	TMap<TWeakObjectPtr<APlayerController>, int32> OuterAssignmentRevisions;
	TMap<TWeakObjectPtr<APlayerState>, TWeakObjectPtr<AGP_EnemySpawnVolume>>
		AssignedOuterZonesByPlayer;
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
	bool bCurrentZoneBossPhaseStarted = false;
	bool bRunStarted = false;
	bool bRunFinished = false;
	bool bPartyPlayerStartsInitialized = false;

	FTimerHandle ReturnToLobbyTimerHandle;

	void EnsurePartyPlayerStarts(AController* Player);
	APlayerStart* SpawnRuntimePartyPlayerStart(
		const APlayerStart& Anchor,
		const APawn* PawnToFit,
		const FVector& LocalDirection,
		float RadiusMultiplier);
	int32 ResolvePartyStartSlot(AController* Player);
	void GatherZones();
	void InitializeZoneProgression();
	void BindZoneDelegates(AGP_EnemySpawnVolume* Zone);
	void ResolveVillageLayoutDirector();
	void ResolveRuntimeRegionConfiguration();
	void InitializeRegionStates();
	void RegisterZoneNavigationInvoker(AGP_EnemySpawnVolume* Zone);
	void UnregisterZoneNavigationInvoker(AGP_EnemySpawnVolume* Zone);
	void UnregisterAllZoneNavigationInvokers();
	void UnlockZone(int32 ZoneIndex);
	void StartZone(int32 ZoneIndex);
	void SpawnZoneEnemies(AGP_EnemySpawnVolume* Zone);
	int32 SpawnZoneBossEnemies(AGP_EnemySpawnVolume* Zone);
	void SpawnMarkerEnemies(AGP_EnemySpawnVolume* Zone, AGP_EnemySpawnMarker* Marker);
	void RegisterZoneEnemy(
		AGP_EnemyCharacter* Enemy,
		AGP_EnemySpawnVolume* OwningZone = nullptr,
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
	bool GetActiveAssignedOuterZones(
		TArray<AGP_EnemySpawnVolume*>& OutAssignedZones) const;
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
	void SpawnMiddleSelectionPortal(
		AGP_EnemySpawnVolume* FromZone,
		const FVector& PreferredSpawnLocation);
	AActor* FindTaggedActorInZoneLevel(
		const AGP_EnemySpawnVolume* Zone,
		FName ActorTag) const;
	bool IsPlayerNearMiddleSelectionPortal(
		const AGP_PlayerController* PlayerController) const;
	void AssignPlayersToOuterVillageStarts();

#if !UE_BUILD_SHIPPING
	// End-to-end QA continues past seamless travel until the authoritative three-player gameplay state is usable.
	void BeginThreePlayerGameplaySmokeProbe();
	void TryThreePlayerGameplaySmokeProbe();
	FTimerHandle ThreePlayerGameplaySmokeTimerHandle;
	int32 ThreePlayerGameplaySmokeAttempts = 0;
#endif

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
	void HandleZoneEnemyDied(AGP_EnemyCharacter* DeadEnemy, AActor* DeathInstigator);

	AGP_GameState* GetGPGameState() const;
};
