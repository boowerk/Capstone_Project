#pragma once

#include "CoreMinimal.h"

class UWorld;

struct PROJECT_EDEN_API FGPGroundPlacementSettings
{
	FVector ProjectionExtent = FVector(300.0f, 300.0f, 300.0f);
	float MaxHeightAboveAnchor = 250.0f;
	int32 MaxAttempts = 8;
};

namespace GPGroundPlacement
{
	// Downward projection is intentionally unrestricted so vertically offset
	// Level Instances can still resolve to their landscape. Only suspicious
	// upward placement (for example a roof nav island) is rejected.
	PROJECT_EDEN_API bool IsGroundRiseWithinLimit(
		const FVector& GroundAnchor,
		const FVector& CandidateGround,
		float MaxRise);

	PROJECT_EDEN_API bool TryProjectGroundAnchor(
		UWorld* World,
		const FVector& DesiredAnchor,
		const FGPGroundPlacementSettings& Settings,
		FVector& OutGroundAnchor);

	PROJECT_EDEN_API bool IsReachableGround(
		UWorld* World,
		const FVector& ProjectedGroundAnchor,
		const FVector& CandidateGround,
		float MaxRise);

	PROJECT_EDEN_API bool TryProjectReachableGround(
		UWorld* World,
		const FVector& ProjectedGroundAnchor,
		const FVector& DesiredCandidate,
		const FGPGroundPlacementSettings& Settings,
		FVector& OutGroundCandidate);

	PROJECT_EDEN_API bool TryGetRandomReachableGround(
		UWorld* World,
		const FVector& ProjectedGroundAnchor,
		float Radius,
		const FGPGroundPlacementSettings& Settings,
		FVector& OutGroundCandidate);
}
