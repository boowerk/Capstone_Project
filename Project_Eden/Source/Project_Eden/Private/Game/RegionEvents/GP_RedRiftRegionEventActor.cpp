#include "Game/RegionEvents/GP_RedRiftRegionEventActor.h"

void AGP_RedRiftRegionEventActor::ActivateRegionEvent()
{
	if (GetRuntimeState() != EGPRegionEventRuntimeState::Dormant)
	{
		return;
	}

	Super::ActivateRegionEvent();

	if (!HasAuthority() || GetRuntimeState() != EGPRegionEventRuntimeState::Active)
	{
		return;
	}

	if (InitialWaveDelaySeconds <= 0.0f)
	{
		SpawnWave();
	}

	if (MaxWaveCount <= 0 || SpawnedWaveCount < MaxWaveCount)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				WaveTimerHandle,
				this,
				&ThisClass::SpawnWave,
				FMath::Max(0.1f, WaveIntervalSeconds),
				true,
				FMath::Max(0.0f, InitialWaveDelaySeconds));
		}
	}
}

void AGP_RedRiftRegionEventActor::CompleteRegionEvent()
{
	StopWaveTimer();
	Super::CompleteRegionEvent();
}

void AGP_RedRiftRegionEventActor::ExpireRegionEvent()
{
	StopWaveTimer();
	Super::ExpireRegionEvent();
}

void AGP_RedRiftRegionEventActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopWaveTimer();
	Super::EndPlay(EndPlayReason);
}

void AGP_RedRiftRegionEventActor::SpawnWave()
{
	if (!HasAuthority() || GetRuntimeState() != EGPRegionEventRuntimeState::Active)
	{
		StopWaveTimer();
		return;
	}

	if (MaxWaveCount > 0 && SpawnedWaveCount >= MaxWaveCount)
	{
		StopWaveTimer();
		return;
	}

	++SpawnedWaveCount;
	// Reuse the base event spawn list so designers tune wave composition from one DataAsset field.
	SpawnConfiguredEnemies();
}

void AGP_RedRiftRegionEventActor::StopWaveTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WaveTimerHandle);
	}
}
