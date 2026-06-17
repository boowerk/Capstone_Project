#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP_RunPortal.generated.h"

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

	// Fired on the server when the portal is triggered, before teleport. Hook VFX/sound here in BP.
	UFUNCTION(BlueprintImplementableEvent, Category = "Portal")
	void OnPortalActivated();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	TObjectPtr<USphereComponent> Trigger;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal")
	FVector TargetLocation = FVector::ZeroVector;

private:
	bool bTriggered = false;

	UFUNCTION()
	void HandleOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
