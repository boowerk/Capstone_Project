#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GP_GameMode.generated.h"

class AGP_EnemyCharacter;
class AGP_EnemySpawnVolume;
class AGP_GameState;
class AGP_RunPortal;
class AGP_EnemySpawnMarker;
class APawn;
class APlayerStart;

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

	virtual void RestartPlayer(AController* NewPlayer) override;

protected:
	virtual void BeginPlay() override;
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
	int32 RegionCount = 9;

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

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<AGP_EnemySpawnVolume>> OrderedZones;

	UPROPERTY(Transient)
	TArray<TObjectPtr<APlayerStart>> RuntimePartyPlayerStarts;

	// Weak slots also include map-authored starts; only spawned starts need a strong transient reference above.
	TArray<TWeakObjectPtr<APlayerStart>> PartyPlayerStartSlots;
	TMap<TWeakObjectPtr<AController>, int32> PartyStartSlotByController;

	// Populated from placed BP_RegionSeed actors when a map authors distinct biome states.
	TArray<uint8> RuntimeInitialRegionStates;

	int32 CurrentZoneIndex = INDEX_NONE;
	int32 PendingZoneIndex = INDEX_NONE;
	int32 AliveZoneEnemies = 0;
	int32 MarkersTotal = 0;
	int32 MarkersTriggered = 0;
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
	void ResolveRuntimeRegionConfiguration();
	void InitializeRegionStates();
	void UnlockZone(int32 ZoneIndex);
	void StartZone(int32 ZoneIndex);
	void SpawnZoneEnemies(AGP_EnemySpawnVolume* Zone);
	void SpawnMarkerEnemies(AGP_EnemySpawnVolume* Zone, AGP_EnemySpawnMarker* Marker);
	void RegisterZoneEnemy(AGP_EnemyCharacter* Enemy);
	void MaybeCompleteZone();
	void CompleteCurrentZone();
	void AdvanceZone();
	void SpawnPortalToZone(int32 FromZoneIndex, int32 ToZoneIndex);
	void FinishRun(bool bVictory);
	void ReturnToLobby();

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
	void HandleMarkerTriggered(AGP_EnemySpawnVolume* Zone, AGP_EnemySpawnMarker* Marker);

	UFUNCTION()
	void HandleZoneEnemyDied(AGP_EnemyCharacter* DeadEnemy, AActor* DeathInstigator);

	AGP_GameState* GetGPGameState() const;
};
