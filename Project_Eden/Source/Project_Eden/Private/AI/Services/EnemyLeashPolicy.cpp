#include "AI/Services/EnemyLeashPolicy.h"

bool EnemyLeashPolicy::ShouldReturnHome(const FEnemyLeashObservation& Observation)
{
	if (!Observation.bEnabled)
	{
		return false;
	}

	const float DistanceFromHome = FMath::Max(0.0f, Observation.DistanceFromHome);
	if (Observation.bCurrentlyReturningHome)
	{
		const float ReengageDistance = FMath::Max(0.0f, Observation.ReengageDistanceFromHome);
		if (Observation.bHasVisibleTarget && DistanceFromHome <= ReengageDistance)
		{
			return false;
		}

		// Keep the return stable until home, unless movement already completed within the fail-safe radius.
		return DistanceFromHome > FMath::Max(0.0f, Observation.ReturnHomeAcceptanceRadius)
			&& !Observation.bReturnMoveStoppedNearHome;
	}

	return DistanceFromHome > FMath::Max(0.0f, Observation.MaxChaseDistanceFromHome);
}
