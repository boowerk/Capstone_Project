#pragma once

#include "CoreMinimal.h"
#include "Game/Demo/GP_DemoRunFlowPolicy.h"
#include "GameFramework/GameModeBase.h"
#include "GP_GameMode.generated.h"

class AGP_EnemyCharacter;
class AGP_CorruptionPresentationActor;
class AGP_DemoRunDirector;
class AGP_EnemySpawnVolume;
class AGP_GameState;
class AGP_RunPortal;
class AGP_EnemySpawnMarker;
class AGP_RegionEventActor;
class AGP_RegionEventDirector;
class APawn;
class APlayerStart;
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
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	int32 GetDemoRunSeed() const { return DemoRunSeed; }
	bool HasValidDemoRunRoute() const { return bHasValidDemoRunRoute; }
	const FGPDemoRunRoute& GetDemoRunRoute() const { return DemoRunRoute; }

	// The guided landscape flow shares the existing result and lobby-return path.
	void CompleteGuidedDemoRun(bool bVictory);

	// Manual trigger — bypasses overlap and immediately spawns zone 0. Use from BP if needed.
	UFUNCTION(BlueprintCallable, Category = "Run")
	void StartRun();

	UFUNCTION(BlueprintCallable, Category = "Run")
	void NotifyAllPlayersDead();

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

	// Only the production open landscape receives the layered graduation-demo golden path.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Demo Flow")
	bool bAutoStartGuidedDemoFlow = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Demo Flow", meta = (EditCondition = "bAutoStartGuidedDemoFlow"))
	TSubclassOf<AGP_DemoRunDirector> DemoRunDirectorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run|Demo Flow", meta = (EditCondition = "bAutoStartGuidedDemoFlow", ClampMin = "0.1", Units = "s"))
	float DemoFlowStartupRetryIntervalSeconds = 0.5f;

	// The production landscape currently authors one anchor. The server expands it into stable,
	// collision-checked slots so a three-player party never relies on spawn collision nudging.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Network|Spawn", meta = (ClampMin = "1"))
	int32 RequiredPartyPlayerStartCount = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Network|Spawn", meta = (ClampMin = "150.0", Units = "cm"))
	float PartyPlayerStartSpacing = 260.0f;

	// Only the production landscape replaces its authored center anchor with a shared peripheral party cluster.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Network|Spawn")
	bool bUsePeripheralPartyStarts = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Network|Spawn", meta = (EditCondition = "bUsePeripheralPartyStarts"))
	FString PeripheralStartMapToken = TEXT("L_LandscapeMap");

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<AGP_EnemySpawnVolume>> OrderedZones;

	UPROPERTY(Transient)
	TObjectPtr<AGP_RegionEventDirector> RegionEventDirector;

	UPROPERTY(Transient)
	TObjectPtr<AGP_DemoRunDirector> DemoRunDirector;

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
	bool bDemoRunSeedInitialized = false;
	bool bHasValidDemoRunRoute = false;
	int32 DemoRunSeed = 0;
	FString InitializedMapName;

	UPROPERTY(Transient)
	FGPDemoRunRoute DemoRunRoute;

	FTimerHandle ReturnToLobbyTimerHandle;
	FTimerHandle DemoFlowStartupTimerHandle;

	void EnsurePartyPlayerStarts(AController* Player);
	void EnsureDemoRunSeed();
	void InitializeDemoRunSeed(const FString& MapName, const FString& Options);
	bool ShouldUsePeripheralPartyStarts() const;
	bool CollectDemoRegionSeedPoints(TArray<FGPDemoRegionSeedPoint>& OutSeedPoints) const;
	bool TryInitializePeripheralPartyStarts(const APawn* PawnToFit);
	bool TryResolvePeripheralClusterTransforms(
		const FGPDemoRunRoute& Route,
		const APawn* PawnToFit,
		TArray<FTransform>& OutTransforms) const;
	bool ResolveSafePeripheralStartTransform(
		const FVector& DesiredLocation,
		const FRotator& DesiredRotation,
		const APawn* PawnToFit,
		FTransform& OutTransform) const;
	bool SpawnPeripheralPartyStartsAtomically(const TArray<FTransform>& StartTransforms);
	APlayerStart* SpawnRuntimePartyPlayerStart(
		const APlayerStart& Anchor,
		const APawn* PawnToFit,
		const FVector& LocalDirection,
		float RadiusMultiplier);
	int32 ResolvePartyStartSlot(AController* Player);
	void GatherZones();
	void ResolveRuntimeRegionConfiguration();
	void ResolveRegionEventDirector();
	void BeginGuidedDemoFlowStartup();
	void TryStartGuidedDemoFlow();
	void InitializeRegionStates();
	void SpawnCorruptionPresentation();
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

	UFUNCTION()
	void HandleRegionEventEnemySpawned(AGP_RegionEventActor* EventActor, AGP_EnemyCharacter* Enemy);

	UFUNCTION()
	void HandleRegionEventEnded(AGP_RegionEventDirector* Director, AGP_RegionEventActor* EventActor);

	AGP_GameState* GetGPGameState() const;
};
