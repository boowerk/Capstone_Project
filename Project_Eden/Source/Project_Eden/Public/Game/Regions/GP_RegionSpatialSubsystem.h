#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GP_RegionSpatialSubsystem.generated.h"

/**
 * Runtime spatial index for the RegionSystem's placed BP_RegionSeed actors.
 * The nearest seed in XY matches the Voronoi partition used to author the region-id texture.
 */
UCLASS()
class PROJECT_EDEN_API UGP_RegionSpatialSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// Rebuilds the cache after streamed levels or editor-authored region seeds change.
	UFUNCTION(BlueprintCallable, Category = "Region|Spatial")
	void RefreshRegionSeeds();

	// Returns INDEX_NONE when the current world has no valid region seed.
	UFUNCTION(BlueprintPure, Category = "Region|Spatial")
	int32 ResolveRegionIdAtLocation(const FVector& WorldLocation);

	// Returns the array size required to address every cached RegionId (highest id + 1).
	UFUNCTION(BlueprintPure, Category = "Region|Spatial")
	int32 GetRegionCount();

	// Returns the number of unique, valid seed actors rather than the addressable id range.
	UFUNCTION(BlueprintPure, Category = "Region|Spatial")
	int32 GetSeedCount();

	// Exposes the authored seed position for event placement and diagnostics.
	UFUNCTION(BlueprintPure, Category = "Region|Spatial")
	bool GetSeedLocation(int32 RegionId, FVector& OutWorldLocation);

private:
	struct FCachedRegionSeed
	{
		int32 RegionId = INDEX_NONE;
		FVector WorldLocation = FVector::ZeroVector;
	};

	TArray<FCachedRegionSeed> CachedSeeds;
	TMap<int32, FVector> CachedSeedLocations;
	int32 CachedRegionCount = 0;
	bool bHasScannedWorld = false;

	void EnsureRegionSeedsCached();
	static bool IsRegionSeedActor(const AActor& Actor);
	static bool TryResolveSeedIndex(const AActor& Actor, int32& OutSeedIndex, bool& bOutHadSeedIndexProperty);
	static bool TryParseTrailingRegionId(const FString& Identifier, int32& OutRegionId);

#if WITH_DEV_AUTOMATION_TESTS
	// The automation test validates the fallback parser without publishing it as gameplay API.
	friend class FGPRegionSpatialSubsystemTest;
#endif
};
