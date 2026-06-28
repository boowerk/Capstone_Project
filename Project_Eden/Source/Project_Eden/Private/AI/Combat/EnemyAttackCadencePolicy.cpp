#include "AI/Combat/EnemyAttackCadencePolicy.h"

FVector2D EnemyAttackCadencePolicy::SanitizeDelayRange(float MinSeconds, float MaxSeconds)
{
	const float SafeMinSeconds = FMath::Clamp(
		FMath::IsFinite(MinSeconds) ? MinSeconds : 0.0f,
		0.0f,
		MaximumDelaySeconds);
	const float SafeMaxSeconds = FMath::Clamp(
		FMath::IsFinite(MaxSeconds) ? MaxSeconds : SafeMinSeconds,
		SafeMinSeconds,
		MaximumDelaySeconds);
	return FVector2D(SafeMinSeconds, SafeMaxSeconds);
}

float EnemyAttackCadencePolicy::RollDelay(const FVector2D& DelayRange, FRandomStream& RandomStream)
{
	const FVector2D SafeRange = SanitizeDelayRange(DelayRange.X, DelayRange.Y);
	// RandomStream is owned by the enemy, so every successful attack advances only that enemy's cadence.
	return RandomStream.FRandRange(SafeRange.X, SafeRange.Y);
}

bool EnemyAttackCadencePolicy::IsReady(float WorldTimeSeconds, float ReadyTimeSeconds)
{
	return FMath::IsFinite(WorldTimeSeconds)
		&& FMath::IsFinite(ReadyTimeSeconds)
		&& WorldTimeSeconds + KINDA_SMALL_NUMBER >= ReadyTimeSeconds;
}
