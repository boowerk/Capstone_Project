#pragma once

#include "CoreMinimal.h"
#include "GP_DemoRunFlowPolicy.generated.h"

/** Authored role of one point in the graduation demo's inward route. */
UENUM(BlueprintType)
enum class EGPDemoRouteRole : uint8
{
	OuterStart UMETA(DisplayName = "Outer Start"),
	OpeningObjective UMETA(DisplayName = "Opening Objective"),
	PressureObjective UMETA(DisplayName = "Pressure Objective"),
	InnerReward UMETA(DisplayName = "Inner Reward"),
	CenterBoss UMETA(DisplayName = "Center Boss")
};

/** Minimal region-seed input kept independent from map and Blueprint actor types. */
USTRUCT(BlueprintType)
struct PROJECT_EDEN_API FGPDemoRegionSeedPoint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo Run")
	int32 RegionId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo Run")
	FVector WorldLocation = FVector::ZeroVector;
};

/** One deterministic waypoint selected from the authored region seeds. */
USTRUCT(BlueprintType)
struct PROJECT_EDEN_API FGPDemoRoutePoint
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Demo Run")
	EGPDemoRouteRole Role = EGPDemoRouteRole::OuterStart;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Demo Run")
	int32 RegionId = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Demo Run")
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Demo Run")
	float NormalizedCenterDistance = 0.0f;
};

/** Complete outer-to-center route shared by player spawning and the runtime flow director. */
USTRUCT(BlueprintType)
struct PROJECT_EDEN_API FGPDemoRunRoute
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Demo Run")
	int32 RunSeed = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Demo Run")
	FVector CenterLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Demo Run")
	TArray<FGPDemoRoutePoint> Points;

	// The ordered outer list lets spawn validation try another authored edge without rerolling the run.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Demo Run")
	TArray<int32> PeripheralCandidateRegionIds;

	bool IsValid() const;
	const FGPDemoRoutePoint* FindPoint(EGPDemoRouteRole Role) const;
};

/** Pure deterministic rules for producing a readable route over irregular RegionSeed layouts. */
namespace GPDemoRunFlowPolicy
{
	PROJECT_EDEN_API bool BuildInwardRoute(
		const TArray<FGPDemoRegionSeedPoint>& SeedPoints,
		int32 RunSeed,
		FGPDemoRunRoute& OutRoute);

	// Spawn validation uses this overload to keep the same run seed while trying the next safe outer candidate.
	PROJECT_EDEN_API bool BuildInwardRouteFromOuterRegion(
		const TArray<FGPDemoRegionSeedPoint>& SeedPoints,
		int32 RunSeed,
		int32 OuterRegionId,
		FGPDemoRunRoute& OutRoute);

	PROJECT_EDEN_API bool IsStrictlyInward(const FGPDemoRunRoute& Route);
}
