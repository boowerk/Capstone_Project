#pragma once

#include "CoreMinimal.h"
#include "EnemyAttackCadencePolicy.generated.h"

/** Designer-facing timing values used to stagger regular enemy attacks. */
USTRUCT(BlueprintType)
struct PROJECT_EDEN_API FGPEnemyAttackCadenceSettings
{
	GENERATED_BODY()

	// A short spawn/acquisition delay keeps a newly alerted group from attacking on the same frame.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Combat|Cadence", meta = (ClampMin = "0.0", ClampMax = "2.95", Units = "s"))
	float InitialDelayMinSeconds = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Combat|Cadence", meta = (ClampMin = "0.0", ClampMax = "2.95", Units = "s"))
	float InitialDelayMaxSeconds = 0.45f;

	// A new value is rolled after every successful attack so equally typed enemies also drift apart naturally.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Combat|Cadence", meta = (ClampMin = "0.0", ClampMax = "2.95", Units = "s"))
	float NextAttackDelayMinSeconds = 1.20f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Combat|Cadence", meta = (ClampMin = "0.0", ClampMax = "2.95", Units = "s"))
	float NextAttackDelayMaxSeconds = 1.80f;
};

namespace EnemyAttackCadencePolicy
{
	// The user-facing contract keeps every regular-enemy interval below the former three-second wait.
	inline constexpr float MaximumDelaySeconds = 2.95f;

	PROJECT_EDEN_API FVector2D SanitizeDelayRange(float MinSeconds, float MaxSeconds);
	PROJECT_EDEN_API float RollDelay(const FVector2D& DelayRange, FRandomStream& RandomStream);
	PROJECT_EDEN_API bool IsReady(float WorldTimeSeconds, float ReadyTimeSeconds);
}
