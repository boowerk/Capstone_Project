#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP_RunPortal.generated.h"

class AGP_PlayerCharacter;
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

	// Set by GameMode at spawn: where players are sent when they enter this portal.
	void SetTargetLocation(const FVector& InTargetLocation) { TargetLocation = InTargetLocation; }

protected:
	virtual void BeginPlay() override;

	// Fired on the server when the last player passes through (portal about to destroy).
	UFUNCTION(BlueprintImplementableEvent, Category = "Portal")
	void OnPortalActivated();

	// Fired on the server each time a player passes through.
	UFUNCTION(BlueprintImplementableEvent, Category = "Portal")
	void OnPlayerPassedThrough(AGP_PlayerCharacter* Player);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	TObjectPtr<USphereComponent> Trigger;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal")
	FVector TargetLocation = FVector::ZeroVector;

private:
	// Tracks which players have already passed through to prevent double-teleport.
	TSet<TObjectPtr<AGP_PlayerCharacter>> PlayersEntered;

	UFUNCTION()
	void HandleOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
