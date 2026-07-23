#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "GP_GameState.generated.h"

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
class UGP_WorldCorruptionComponent;
class APlayerState;

=======
>>>>>>> origin/main
=======
>>>>>>> origin/main
=======
>>>>>>> origin/main
// Replicated phase of the linear city-progression run. Drives HUD and client-side presentation.
UENUM(BlueprintType)
enum class EGPMatchPhase : uint8
{
	Preparing UMETA(DisplayName = "Preparing"),
	InCity UMETA(DisplayName = "In City"),
	BossFight UMETA(DisplayName = "Boss Fight"),
	Transition UMETA(DisplayName = "Transition"),
	Victory UMETA(DisplayName = "Victory"),
	Defeat UMETA(DisplayName = "Defeat")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGPOnMatchPhaseChanged, EGPMatchPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGPOnEnemiesRemainingChanged, int32, EnemiesRemaining);

// Fired on every machine when region states change. RegionId == INDEX_NONE (-1)
// means "all regions reset" (e.g. initial all-dead); otherwise it is the single
// region whose state changed. The BP RegionStateManager binds this to push the
// state into its render target and regenerate that region's vegetation PCG.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGPOnRegionStateChanged, int32, RegionId, uint8, NewState);

/**
 * Replicated run state for the linear city-progression mode. The GameMode (server) writes these values;
 * clients read them for HUD ("Zone 2", "BOSS", remaining enemy count). See the PCG/gameplay-flow topic note.
 */
UCLASS()
class PROJECT_EDEN_API AGP_GameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Run")
	EGPMatchPhase GetMatchPhase() const { return MatchPhase; }

	UFUNCTION(BlueprintPure, Category = "Run")
	int32 GetCurrentZoneIndex() const { return CurrentZoneIndex; }

	UFUNCTION(BlueprintPure, Category = "Run")
	int32 GetEnemiesRemaining() const { return EnemiesRemaining; }

	UFUNCTION(BlueprintPure, Category = "Run")
	bool HasRunSeed() const { return RunSeed >= 0; }

	UFUNCTION(BlueprintPure, Category = "Run")
	int32 GetRunSeed() const { return RunSeed; }

	UFUNCTION(BlueprintPure, Category = "Run|Progression")
	bool HasMiddleClearCredit(const APlayerState* PlayerState) const;

	UFUNCTION(BlueprintPure, Category = "Run|Progression")
	int32 GetMiddleClearCreditCount() const { return MiddleQualifiedPlayers.Num(); }

	UFUNCTION(BlueprintPure, Category = "Run|Region")
	uint8 GetRegionState(int32 RegionId) const;

	UFUNCTION(BlueprintPure, Category = "Run|Region")
	int32 GetRegionCount() const { return RegionStates.Num(); }

	// Server-only setters, called by AGP_GameMode as the run progresses.
	void SetMatchPhase(EGPMatchPhase NewPhase);
	void SetCurrentZoneIndex(int32 NewZoneIndex);
	void SetEnemiesRemaining(int32 NewEnemiesRemaining);
	void SetRunSeed(int32 NewRunSeed);
	void GrantMiddleClearCredit(APlayerState* PlayerState);

	// Server-only. Resizes RegionStates to Count and fills it with InitialState
	// (e.g. all regions dead at run start). Broadcasts a reset (-1).
	void InitRegionStates(int32 Count, uint8 InitialState);

	// Server-only. Preserves an authored per-region biome array instead of flattening every region to one state.
	void InitRegionStatesFromArray(const TArray<uint8>& InitialStates);

	// Server-only. Sets one region's state (e.g. revive on zone clear).
	void SetRegionState(int32 RegionId, uint8 NewState);

	// Bound by the RegionStateManager (BP) to drive landscape/vegetation updates.
	UPROPERTY(BlueprintAssignable, Category = "Run|Region")
	FGPOnRegionStateChanged OnRegionStateChanged;

	// Fired on every machine (server via setters, clients via OnRep) when the matching value changes.
	UPROPERTY(BlueprintAssignable, Category = "Run")
	FGPOnMatchPhaseChanged OnMatchPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "Run")
	FGPOnEnemiesRemainingChanged OnEnemiesRemainingChanged;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_MatchPhase, BlueprintReadOnly, Category = "Run", meta = (AllowPrivateAccess = "true"))
	EGPMatchPhase MatchPhase = EGPMatchPhase::Preparing;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentZoneIndex, BlueprintReadOnly, Category = "Run", meta = (AllowPrivateAccess = "true"))
	int32 CurrentZoneIndex = 0;

	UPROPERTY(ReplicatedUsing = OnRep_EnemiesRemaining, BlueprintReadOnly, Category = "Run", meta = (AllowPrivateAccess = "true"))
	int32 EnemiesRemaining = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Run", meta = (AllowPrivateAccess = "true"))
	int32 RunSeed = INDEX_NONE;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Run|Progression", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<APlayerState>> MiddleQualifiedPlayers;

	// Per-region state value (index = region id). Replicated; clients apply it
	// to the landscape/vegetation via OnRegionStateChanged.
	UPROPERTY(ReplicatedUsing = OnRep_RegionStates, BlueprintReadOnly, Category = "Run|Region", meta = (AllowPrivateAccess = "true"))
	TArray<uint8> RegionStates;

	// Snapshot of the previous replicated array so OnRep can tell which region(s) changed.
	TArray<uint8> LocalRegionStatesShadow;

	UFUNCTION()
	void OnRep_MatchPhase();

	UFUNCTION()
	void OnRep_CurrentZoneIndex();

	UFUNCTION()
	void OnRep_EnemiesRemaining();

	UFUNCTION()
	void OnRep_RegionStates();

	// Emits one compact reset for uniform arrays and per-region changes for authored heterogeneous biome arrays.
	void BroadcastFullRegionState();
};
