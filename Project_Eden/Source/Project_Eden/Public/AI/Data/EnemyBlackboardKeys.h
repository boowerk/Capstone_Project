#pragma once

#include "CoreMinimal.h"

namespace EnemyBlackboardKeys
{
	PROJECT_EDEN_API extern const FName TargetActor;
	// BT에서 적의 현재 논리 상태 태그 이름을 비교할 때 사용하는 키다.
	PROJECT_EDEN_API extern const FName CombatState;
	PROJECT_EDEN_API extern const FName EnemyMode;
	PROJECT_EDEN_API extern const FName Aggression;
	PROJECT_EDEN_API extern const FName PreferredRange;
	PROJECT_EDEN_API extern const FName RetreatThreshold;
	PROJECT_EDEN_API extern const FName ChasePersistence;
	PROJECT_EDEN_API extern const FName CoverPreference;
	PROJECT_EDEN_API extern const FName HomeLocation;
	PROJECT_EDEN_API extern const FName MoveToLocation;
	PROJECT_EDEN_API extern const FName FocusTargetRule;
	PROJECT_EDEN_API extern const FName bShouldRetreat;
	PROJECT_EDEN_API extern const FName bCanAttack;
	PROJECT_EDEN_API extern const FName bShouldReposition;
	PROJECT_EDEN_API extern const FName bShouldChase;
	PROJECT_EDEN_API extern const FName bShouldReturnHome;
	PROJECT_EDEN_API extern const FName bHasLineOfSight;
	PROJECT_EDEN_API extern const FName DistanceToTarget;
	PROJECT_EDEN_API extern const FName DistanceFromHome;
	PROJECT_EDEN_API extern const FName HealthRatio;
}
