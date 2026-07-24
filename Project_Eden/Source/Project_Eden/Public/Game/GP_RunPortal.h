#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP_RunPortal.generated.h"

class AGP_PlayerCharacter;
class AGP_EnemySpawnVolume;
class APlayerState;
class USphereComponent;

/**
 * Teleport device spawned by AGP_GameMode when a zone is cleared. When any player steps into it,
 * all player characters in the level are teleported to TargetLocation (the next zone's box), then
 * the portal deactivates. Same-map travel only — no level load, so GameMode/GameState/PlayerState
 * (XP, augments, skills) all survive. Mesh/VFX are added in a Blueprint child via OnPortalActivated.
 */
UCLASS()
class PROJECT_EDEN_API AGP_RunPortal : public AActor
{
	GENERATED_BODY()

public:
	AGP_RunPortal();
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Set by GameMode at spawn. Keeping the authoritative destination zone lets
	// progression distinguish a legitimate portal arrival from a debug teleport
	// or an ordinary overlap at the same world position.
	void SetDestination(
		AGP_EnemySpawnVolume* InTargetZone,
		const FVector& InTargetLocation);
	void SetMiddleDestinationSelection(bool bEnabled);
	bool IsMiddleDestinationSelection() const { return bMiddleDestinationSelection; }
	bool IsDestinationConfigured() const { return bDestinationConfigured; }
	static bool IsActivePartyComplete(
		int32 ActivePlayerCount,
		int32 EnteredActivePlayerCount);

protected:
	virtual void BeginPlay() override;

	// Fired on the server when the last player passes through (portal about to destroy).
	UFUNCTION(BlueprintImplementableEvent, Category = "Portal")
	void OnPortalActivated();

	// Fired on the server each time a player passes through.
	UFUNCTION(BlueprintImplementableEvent, Category = "Portal")
	void OnPlayerPassedThrough(AGP_PlayerCharacter* Player);

	// Fired for the owning player when this portal opens the Middle destination map.
	UFUNCTION(BlueprintImplementableEvent, Category = "Portal")
	void OnMiddleDestinationSelectionOpened(AGP_PlayerCharacter* Player);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	TObjectPtr<USphereComponent> Trigger;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal")
	FVector TargetLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	TObjectPtr<AGP_EnemySpawnVolume> TargetZone;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	bool bMiddleDestinationSelection = false;

private:
	// PlayerState identity survives pawn replacement and lets stale disconnected
	// entries be ignored when comparing against the current active party.
	TSet<TWeakObjectPtr<APlayerState>> PlayersEntered;
	bool bDestinationConfigured = false;

	UFUNCTION()
	void HandleOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	void HandleExistingOverlaps();
	void TryHandlePlayer(AGP_PlayerCharacter* Player);
	bool HaveAllActivePlayersEntered() const;
};
