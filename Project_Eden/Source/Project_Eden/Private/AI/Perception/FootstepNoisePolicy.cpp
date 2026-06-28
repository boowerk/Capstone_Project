#include "AI/Perception/FootstepNoisePolicy.h"

float FootstepNoisePolicy::ResolveInterval(const FGPFootstepNoiseSettings& Settings, bool bSprinting)
{
	const float AuthoredInterval = bSprinting ? Settings.SprintNoiseInterval : Settings.WalkNoiseInterval;
	return FMath::Max(FMath::IsFinite(AuthoredInterval) ? AuthoredInterval : 0.05f, 0.05f);
}

float FootstepNoisePolicy::ResolveLoudness(const FGPFootstepNoiseSettings& Settings, bool bSprinting, bool bCrouched)
{
	const float AuthoredLoudness = bSprinting ? Settings.SprintLoudness : Settings.WalkLoudness;
	const float SafeLoudness = FMath::Max(FMath::IsFinite(AuthoredLoudness) ? AuthoredLoudness : 0.0f, 0.0f);
	const float CrouchMultiplier = bCrouched
		? FMath::Clamp(Settings.CrouchLoudnessMultiplier, 0.0f, 1.0f)
		: 1.0f;
	return SafeLoudness * CrouchMultiplier;
}

bool FootstepNoisePolicy::ShouldReportNoise(
	const FGPFootstepNoiseSettings& Settings,
	float HorizontalSpeed,
	bool bMovingOnGround,
	bool bSprinting,
	float WorldTimeSeconds,
	float LastNoiseTimeSeconds)
{
	if (!bMovingOnGround || !FMath::IsFinite(HorizontalSpeed) || !FMath::IsFinite(WorldTimeSeconds))
	{
		return false;
	}

	const float MinimumSpeed = FMath::Max(
		FMath::IsFinite(Settings.MinimumHorizontalSpeed) ? Settings.MinimumHorizontalSpeed : 0.0f,
		0.0f);
	if (HorizontalSpeed < MinimumSpeed)
	{
		return false;
	}

	// A non-finite previous timestamp is treated as a fresh movement start and reports immediately.
	return !FMath::IsFinite(LastNoiseTimeSeconds)
		|| WorldTimeSeconds - LastNoiseTimeSeconds >= ResolveInterval(Settings, bSprinting);
}
