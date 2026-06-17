#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "GP_GameState.generated.h"

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

/**
 * Replicated run state for the linear city-progression mode. The GameMode (server) writes these values;
 * clients read them for HUD ("Zone 2", "BOSS", remaining enemy count). See the PCG/gameplay-flow topic note.
 */
UCLASS()
class PROJECT_EDEN_API AGP_GameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AGP_GameState();

	UFUNCTION(BlueprintPure, Category = "Run")
	EGPMatchPhase GetMatchPhase() const { return MatchPhase; }

	UFUNCTION(BlueprintPure, Category = "Run")
	int32 GetCurrentZoneIndex() const { return CurrentZoneIndex; }

	UFUNCTION(BlueprintPure, Category = "Run")
	int32 GetEnemiesRemaining() const { return EnemiesRemaining; }

	// Server-only setters, called by AGP_GameMode as the run progresses.
	void SetMatchPhase(EGPMatchPhase NewPhase);
	void SetCurrentZoneIndex(int32 NewZoneIndex);
	void SetEnemiesRemaining(int32 NewEnemiesRemaining);

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

	UFUNCTION()
	void OnRep_MatchPhase();

	UFUNCTION()
	void OnRep_CurrentZoneIndex();

	UFUNCTION()
	void OnRep_EnemiesRemaining();
};
