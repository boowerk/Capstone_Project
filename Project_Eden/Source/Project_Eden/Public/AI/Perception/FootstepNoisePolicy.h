#pragma once

#include "CoreMinimal.h"
#include "FootstepNoisePolicy.generated.h"

/** Server-side movement thresholds used when emitting AI hearing stimuli. */
USTRUCT(BlueprintType)
struct PROJECT_EDEN_API FGPFootstepNoiseSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Footsteps", meta = (ClampMin = "0.0", Units = "cm/s"))
	float MinimumHorizontalSpeed = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Footsteps", meta = (ClampMin = "0.05", Units = "s"))
	float WalkNoiseInterval = 0.42f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Footsteps", meta = (ClampMin = "0.05", Units = "s"))
	float SprintNoiseInterval = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Footsteps", meta = (ClampMin = "0.0"))
	float WalkLoudness = 0.80f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Footsteps", meta = (ClampMin = "0.0"))
	float SprintLoudness = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Footsteps", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CrouchLoudnessMultiplier = 0.35f;

	// Zero leaves the final radius entirely to each enemy's HearingRange setting.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Footsteps", meta = (ClampMin = "0.0", Units = "cm"))
	float MaximumNoiseRange = 0.0f;
};

namespace FootstepNoisePolicy
{
	PROJECT_EDEN_API float ResolveInterval(const FGPFootstepNoiseSettings& Settings, bool bSprinting);
	PROJECT_EDEN_API float ResolveLoudness(const FGPFootstepNoiseSettings& Settings, bool bSprinting, bool bCrouched);
	PROJECT_EDEN_API bool ShouldReportNoise(
		const FGPFootstepNoiseSettings& Settings,
		float HorizontalSpeed,
		bool bMovingOnGround,
		bool bSprinting,
		float WorldTimeSeconds,
		float LastNoiseTimeSeconds);
}
