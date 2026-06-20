#pragma once

#include "CoreMinimal.h"

/** Immutable observations used to decide whether the AI leash owns movement this tick. */
struct FEnemyLeashObservation
{
	bool bEnabled = true;
	bool bCurrentlyReturningHome = false;
	bool bHasVisibleTarget = false;
	bool bReturnMoveStoppedNearHome = false;
	float DistanceFromHome = 0.0f;
	float MaxChaseDistanceFromHome = 0.0f;
	float ReengageDistanceFromHome = 0.0f;
	float ReturnHomeAcceptanceRadius = 0.0f;
};

namespace EnemyLeashPolicy
{
	// The outer boundary starts return; the inner boundary allows a visible player to re-open combat without oscillation.
	PROJECT_EDEN_API bool ShouldReturnHome(const FEnemyLeashObservation& Observation);
}
