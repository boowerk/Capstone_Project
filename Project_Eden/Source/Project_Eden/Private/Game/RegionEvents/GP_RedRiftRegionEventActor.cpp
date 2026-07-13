#include "Game/RegionEvents/GP_RedRiftRegionEventActor.h"

void AGP_RedRiftRegionEventActor::ActivateRegionEvent()
{
	if (GetRuntimeState() != EGPRegionEventRuntimeState::Dormant)
	{
		return;
	}

	// Reset counters before activation so a pooled/test actor cannot inherit an earlier encounter's wave state.
	SpawnedWaveCount = 0;
	bAllConfiguredWavesSpawned = false;
	Super::ActivateRegionEvent();

	if (!HasAuthority() || GetRuntimeState() != EGPRegionEventRuntimeState::Active)
	{
		return;
	}

	// The base activation already spawns the first configured enemy wave.
	SpawnedWaveCount = 1;

	if (MaxWaveCount > 0 && SpawnedWaveCount >= MaxWaveCount)
	{
		MarkAllConfiguredWavesSpawned();
		return;
	}

	// Non-positive caps intentionally retain the original infinite-wave behavior until duration/explicit expiry.
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

void AGP_RedRiftRegionEventActor::OnTrackedEnemyCountChanged()
{
	Super::OnTrackedEnemyCountChanged();
	// Enemy death callbacks are authoritative, so the final kill can complete the event without polling.
	TryCompleteAfterFinalWave();
}

bool AGP_RedRiftRegionEventActor::ShouldCompleteAfterWaveSequence(
	int32 InMaxWaveCount,
	int32 InSpawnedWaveCount,
	int32 InAliveEnemyCount)
{
	return InMaxWaveCount > 0
		&& InSpawnedWaveCount >= InMaxWaveCount
		&& InAliveEnemyCount <= 0;
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
		MarkAllConfiguredWavesSpawned();
		return;
	}

	++SpawnedWaveCount;
	// Reuse the base event spawn list so designers tune wave composition from one DataAsset field.
	SpawnConfiguredEnemies();

	if (MaxWaveCount > 0 && SpawnedWaveCount >= MaxWaveCount)
	{
		// Stop immediately after the actual final spawn instead of waiting for one redundant timer tick.
		MarkAllConfiguredWavesSpawned();
	}
}

void AGP_RedRiftRegionEventActor::StopWaveTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WaveTimerHandle);
	}
}

void AGP_RedRiftRegionEventActor::MarkAllConfiguredWavesSpawned()
{
	bAllConfiguredWavesSpawned = true;
	StopWaveTimer();
	TryCompleteAfterFinalWave();
}

void AGP_RedRiftRegionEventActor::TryCompleteAfterFinalWave()
{
	if (!HasAuthority()
		|| GetRuntimeState() != EGPRegionEventRuntimeState::Active
		|| !bAllConfiguredWavesSpawned)
	{
		return;
	}

	if (ShouldCompleteAfterWaveSequence(MaxWaveCount, SpawnedWaveCount, GetAliveSpawnedEnemyCount()))
	{
		CompleteRegionEvent();
	}
}
