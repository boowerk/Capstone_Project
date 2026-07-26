#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Game/GP_EnemySpawnVolume.h"
#include "GP_EnemySpawnMarker.generated.h"

class USphereComponent;

/**
 * Standalone encounter marker placed inside a city zone box. AGP_EnemySpawnVolume collects all
 * markers whose centers fall inside its box at BeginPlay. When a player enters the marker's trigger
 * sphere (after the zone activates), that marker's enemies spawn nearby. Place as many as needed
 * per city — no Blueprint subclassing required.
 */
UCLASS()
class PROJECT_EDEN_API AGP_EnemySpawnMarker : public AActor
{
	GENERATED_BODY()

public:
	AGP_EnemySpawnMarker();

	const TArray<FGP_EnemySpawnEntry>& GetSpawns() const { return Spawns; }
	float GetSpawnScatterRadius() const { return SpawnScatterRadius; }
	USphereComponent* GetTrigger() const { return Trigger; }

	// Arm the overlap so the owning zone can react to player entry.
	void Activate(AGP_EnemySpawnVolume* OwningZone);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Marker")
	TObjectPtr<USphereComponent> Trigger;

	// Enemies that spawn near this marker when a player approaches.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Marker")
	TArray<FGP_EnemySpawnEntry> Spawns;

	// Random scatter radius around this marker for spawned enemies.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Marker", meta = (ClampMin = "0.0"))
	float SpawnScatterRadius = 250.0f;

private:
	bool bActive = false;
	bool bTriggered = false;

	UPROPERTY()
	TObjectPtr<AGP_EnemySpawnVolume> OwnerZone = nullptr;

	bool TryTriggerForActor(AActor* OtherActor);
	void EvaluateExistingOverlaps();

	UFUNCTION()
	void HandleOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
