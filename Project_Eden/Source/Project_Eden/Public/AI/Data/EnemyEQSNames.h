#pragma once

#include "CoreMinimal.h"

namespace EnemyEQSNames
{
	// EQS 쿼리 에셋에서 선호 교전 거리를 받는 named param 이름이다.
	PROJECT_EDEN_API extern const FName PreferredRangeParam;

	// EQS 쿼리 에셋에서 엄폐 선호도를 받는 named param 이름이다.
	PROJECT_EDEN_API extern const FName CoverPreferenceParam;

	// EQS 쿼리 에셋에서 공격 성향을 받는 named param 이름이다.
	PROJECT_EDEN_API extern const FName AggressionParam;

	// EQS 쿼리 에셋에서 후퇴 임계값을 받는 named param 이름이다.
	PROJECT_EDEN_API extern const FName RetreatThresholdParam;
}
