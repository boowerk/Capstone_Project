#include "Navigation/GP_GroundPlacement.h"

#include "NavigationSystem.h"

namespace
{
	const UNavigationSystemV1* GetNavigationSystem(UWorld* World)
	{
		return IsValid(World)
			? UNavigationSystemV1::GetCurrent(World)
			: nullptr;
	}

	bool HasPathBetween(
		const UNavigationSystemV1& NavigationSystem,
		const FVector& Start,
		const FVector& End)
	{
		if (Start.Equals(End, KINDA_SMALL_NUMBER))
		{
			return true;
		}

		FVector::FReal PathLength = 0.0;
		return NavigationSystem.GetPathLength(Start, End, PathLength)
			== ENavigationQueryResult::Success;
	}
}

bool GPGroundPlacement::IsGroundRiseWithinLimit(
	const FVector& GroundAnchor,
	const FVector& CandidateGround,
	float MaxRise)
{
	return !GroundAnchor.ContainsNaN()
		&& !CandidateGround.ContainsNaN()
		&& CandidateGround.Z
			<= GroundAnchor.Z + FMath::Max(0.0f, MaxRise);
}

bool GPGroundPlacement::TryProjectGroundAnchor(
	UWorld* World,
	const FVector& DesiredAnchor,
	const FGPGroundPlacementSettings& Settings,
	FVector& OutGroundAnchor)
{
	OutGroundAnchor = DesiredAnchor;
	const UNavigationSystemV1* NavigationSystem = GetNavigationSystem(World);
	if (!NavigationSystem || DesiredAnchor.ContainsNaN())
	{
		return false;
	}

	FNavLocation ProjectedLocation;
	if (!NavigationSystem->ProjectPointToNavigation(
			DesiredAnchor,
			ProjectedLocation,
			Settings.ProjectionExtent)
		|| !IsGroundRiseWithinLimit(
			DesiredAnchor,
			ProjectedLocation.Location,
			Settings.MaxHeightAboveAnchor))
	{
		return false;
	}

	OutGroundAnchor = ProjectedLocation.Location;
	return true;
}

bool GPGroundPlacement::IsReachableGround(
	UWorld* World,
	const FVector& ProjectedGroundAnchor,
	const FVector& CandidateGround,
	float MaxRise)
{
	const UNavigationSystemV1* NavigationSystem = GetNavigationSystem(World);
	return NavigationSystem
		&& IsGroundRiseWithinLimit(
			ProjectedGroundAnchor,
			CandidateGround,
			MaxRise)
		&& HasPathBetween(
			*NavigationSystem,
			ProjectedGroundAnchor,
			CandidateGround);
}

bool GPGroundPlacement::TryProjectReachableGround(
	UWorld* World,
	const FVector& ProjectedGroundAnchor,
	const FVector& DesiredCandidate,
	const FGPGroundPlacementSettings& Settings,
	FVector& OutGroundCandidate)
{
	OutGroundCandidate = DesiredCandidate;
	const UNavigationSystemV1* NavigationSystem = GetNavigationSystem(World);
	if (!NavigationSystem
		|| ProjectedGroundAnchor.ContainsNaN()
		|| DesiredCandidate.ContainsNaN())
	{
		return false;
	}

	FNavLocation ProjectedLocation;
	if (!NavigationSystem->ProjectPointToNavigation(
			DesiredCandidate,
			ProjectedLocation,
			Settings.ProjectionExtent)
		|| !IsReachableGround(
			World,
			ProjectedGroundAnchor,
			ProjectedLocation.Location,
			Settings.MaxHeightAboveAnchor))
	{
		return false;
	}

	OutGroundCandidate = ProjectedLocation.Location;
	return true;
}

bool GPGroundPlacement::TryGetRandomReachableGround(
	UWorld* World,
	const FVector& ProjectedGroundAnchor,
	float Radius,
	const FGPGroundPlacementSettings& Settings,
	FVector& OutGroundCandidate)
{
	OutGroundCandidate = ProjectedGroundAnchor;
	const UNavigationSystemV1* NavigationSystem = GetNavigationSystem(World);
	if (!NavigationSystem || ProjectedGroundAnchor.ContainsNaN())
	{
		return false;
	}

	const float ReachableRadius = FMath::Max(0.0f, Radius);
	if (ReachableRadius <= KINDA_SMALL_NUMBER)
	{
		return true;
	}

	const int32 AttemptCount = FMath::Max(1, Settings.MaxAttempts);
	for (int32 AttemptIndex = 0; AttemptIndex < AttemptCount; ++AttemptIndex)
	{
		FNavLocation ReachableLocation;
		if (NavigationSystem->GetRandomReachablePointInRadius(
				ProjectedGroundAnchor,
				ReachableRadius,
				ReachableLocation)
			&& IsGroundRiseWithinLimit(
				ProjectedGroundAnchor,
				ReachableLocation.Location,
				Settings.MaxHeightAboveAnchor))
		{
			OutGroundCandidate = ReachableLocation.Location;
			return true;
		}
	}

	return false;
}
