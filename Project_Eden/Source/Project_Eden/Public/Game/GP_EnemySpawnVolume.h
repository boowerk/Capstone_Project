#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP_EnemySpawnVolume.generated.h"

class AGP_EnemyCharacter;
class AGP_EnemySpawnMarker;
class AActor;
class APlayerState;
class UBoxComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGPOnPlayerEnteredZone, AGP_EnemySpawnVolume*, Zone);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FGPOnPlayerEnteredZoneDetailed,
	AGP_EnemySpawnVolume*, Zone,
	APlayerState*, PlayerState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FGPOnPlayerExitedZoneDetailed,
	AGP_EnemySpawnVolume*, Zone,
	APlayerState*, PlayerState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGPOnMarkerTriggered, AGP_EnemySpawnVolume*, Zone, AGP_EnemySpawnMarker*, Marker);

UENUM(BlueprintType)
enum class EGPZoneStage : uint8
{
	Legacy UMETA(DisplayName = "Legacy Linear"),
	Outer UMETA(DisplayName = "Outer"),
	Middle UMETA(DisplayName = "Middle"),
	Center UMETA(DisplayName = "Center"),
	Colosseum UMETA(DisplayName = "Colosseum")
};

/** One enemy type and how many of it spawn. */
USTRUCT(BlueprintType)
struct FGP_EnemySpawnEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	TSubclassOf<AGP_EnemyCharacter> EnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (ClampMin = "1"))
	int32 Count = 1;
};

/**
 * Placed-in-level actor defining one progression zone (a city or a boss room).
 *
 * Two spawn modes:
 *  - Marker mode: place AGP_EnemySpawnMarker actors inside the box. This zone collects them at
 *    BeginPlay. Enemies spawn marker-by-marker as the player advances into the city.
 *    Zone clears only when every marker has triggered and all enemies are dead.
 *  - Box-fallback mode: if no markers exist, the flat Spawns array spawns all at once on zone entry
 *    (boss zones always use this mode — center spawn, no markers needed).
 */
UCLASS()
class PROJECT_EDEN_API AGP_EnemySpawnVolume : public AActor
{
	GENERATED_BODY()

public:
	AGP_EnemySpawnVolume();

	int32 GetZoneOrder() const { return ZoneOrder; }
	FName GetZoneId() const { return ZoneId; }
	EGPZoneStage GetZoneStage() const { return ZoneStage; }
	bool IsBossZone() const { return bIsBossZone; }
	FText GetDisplayName() const { return DisplayName; }
	const TArray<int32>& GetRegionsToRevive() const { return RegionsToRevive; }
	int32 GetCorruptionRegionId() const;
	const TArray<FGP_EnemySpawnEntry>& GetSpawns() const { return Spawns; }
	const TArray<FGP_EnemySpawnEntry>& GetBossSpawns() const { return BossSpawns; }
	bool HasBossPhase() const { return !BossSpawns.IsEmpty(); }
	const TArray<TObjectPtr<AGP_EnemySpawnMarker>>& GetMarkers() const { return Markers; }
	int32 GetMarkerCount() const { return Markers.Num(); }

	void Unlock();
	void ActivateMarkers();
	void ConfigureRuntimeZone(
		FName InZoneId,
		EGPZoneStage InZoneStage,
		int32 InZoneOrder,
		int32 InRegionId);

	// Called by AGP_EnemySpawnMarker when a player enters its radius.
	void NotifyMarkerTriggered(AGP_EnemySpawnMarker* Marker);

	FVector GetSpawnPoint(bool bRandomizeInVolume, bool& bOutProjected) const;
	FVector GetBossSpawnPoint(bool& bOutProjected) const;
	FVector GetSpawnPointNearMarker(const AGP_EnemySpawnMarker* Marker, bool& bOutProjected) const;
	FVector GetSpawnPointNearLocation(
		const FVector& AnchorLocation,
		float ScatterRadius,
		bool& bOutProjected) const;
	bool IsSpawnGroundLocationValid(
		const FVector& GroundAnchor,
		const FVector& CandidateGroundLocation) const;
	bool IsFinalSpawnGroundLocationValid(
		const FVector& RequestedGroundLocation,
		const FVector& ActualGroundLocation) const;

	UPROPERTY(BlueprintAssignable, Category = "Zone")
	FGPOnPlayerEnteredZone OnPlayerEnteredZone;

	UPROPERTY(BlueprintAssignable, Category = "Zone")
	FGPOnPlayerEnteredZoneDetailed OnPlayerEnteredZoneDetailed;

	UPROPERTY(BlueprintAssignable, Category = "Zone")
	FGPOnPlayerExitedZoneDetailed OnPlayerExitedZoneDetailed;

	UPROPERTY(BlueprintAssignable, Category = "Zone")
	FGPOnMarkerTriggered OnMarkerTriggered;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zone")
	TObjectPtr<UBoxComponent> SpawnBox;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	int32 ZoneOrder = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	FName ZoneId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	EGPZoneStage ZoneStage = EGPZoneStage::Legacy;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	bool bIsBossZone = false;

	// Region ids that revive (turn alive) when this zone is cleared. Lets a city
	// clear light up its surrounding nature regions. Empty = revives nothing.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone|Region")
	TArray<int32> RegionsToRevive;

	// -1 automatically uses the first revived region, then ZoneOrder as a final fallback.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone|Corruption", meta = (ClampMin = "-1"))
	int32 CorruptionRegionId = INDEX_NONE;

	// Box-fallback composition (used when no markers exist, or for boss zones).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	TArray<FGP_EnemySpawnEntry> Spawns;

	// Optional second phase. When configured, the zone waits for every normal enemy and marker
	// to clear, then spawns this composition at the actor tagged BossSpawnPoint.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone|Boss")
	TArray<FGP_EnemySpawnEntry> BossSpawns;

	// Place Target Point actors with these tags inside the same Level Instance as this zone.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone|Spawn Points")
	FName EnemySpawnPointTag = TEXT("EnemySpawnPoint");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone|Spawn Points")
	FName BossSpawnPointTag = TEXT("BossSpawnPoint");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone|Spawn Points",
		meta = (ClampMin = "0.0", Units = "cm"))
	float EnemySpawnPointScatterRadius = 300.0f;

	// Backup guard for connected NavMesh that climbs onto nearby geometry.
	// Downward projection remains unrestricted so vertically offset Level Instances still recover.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone|Spawn Points",
		meta = (ClampMin = "0.0", Units = "cm"))
	float MaxSpawnHeightAboveGroundAnchor = 250.0f;

	// Collision adjustment may move a capsule slightly above its requested nav
	// point, but must never lift an enemy onto another actor or a roof.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone|Spawn Points",
		meta = (ClampMin = "0.0", Units = "cm"))
	float MaxFinalSpawnHeightAboveNavmesh = 50.0f;

private:
	UPROPERTY()
	TArray<TObjectPtr<AGP_EnemySpawnMarker>> Markers;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> EnemySpawnPoints;

	UPROPERTY()
	TObjectPtr<AActor> BossSpawnPoint;

	bool bUnlocked = false;
	bool bEntered = false;

	bool TryGetReachableSpawnPoint(
		const FVector& DesiredAnchor,
		float ScatterRadius,
		const FVector& ProjectionExtent,
		FVector& OutSpawnLocation) const;
	bool IsNavLocationReachableFromGroundAnchor(
		const FVector& CandidateLocation) const;
	bool IsPointInsideSpawnBox(const FVector& WorldLocation) const;
	bool IsPointInsideSpawnBox2D(const FVector& WorldLocation) const;
	void CollectTaggedSpawnPoints();

	UFUNCTION()
	void HandleBoxOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleBoxEndOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);
};
