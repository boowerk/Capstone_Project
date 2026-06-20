#pragma once

#include "CoreMinimal.h"

/** Immutable observations used to decide whether the AI leash owns movement this tick. */
struct FEnemyLeashObservation
{
	bool bEnabled = true;
	bool bCurrentlyReturningHome = false;
	bool bHasActiveTarget = false;
	bool bReturnMoveStoppedNearHome = false;
	float DistanceFromHome = 0.0f;
	float MaxChaseDistanceFromHome = 0.0f;
	float ReturnHomeAcceptanceRadius = 0.0f;
};

namespace EnemyLeashPolicy
{
	// Active combat always wins; the leash starts only after the enemy loses its target outside the allowed area.
	PROJECT_EDEN_API bool ShouldReturnHome(const FEnemyLeashObservation& Observation);
}
