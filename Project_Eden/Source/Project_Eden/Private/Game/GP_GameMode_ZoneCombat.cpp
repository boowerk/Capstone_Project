#include "Game/GP_GameMode.h"

#include "Characters/GP_EnemyCharacter.h"
#include "Engine/World.h"
#include "Game/GP_EnemySpawnMarker.h"
#include "Game/GP_EnemySpawnVolume.h"
#include "Game/GP_GameState.h"

void AGP_GameMode::HandleMarkerTriggered(AGP_EnemySpawnVolume* Zone, AGP_EnemySpawnMarker* Marker)
{
	if (bRunFinished || !IsValid(Zone) || !Marker)
	{
		return;
	}

	if (bUseStagedZoneProgression)
	{
		FGPZoneRuntimeState* State = ZoneRuntimeStates.Find(Zone);
		if (!State || !State->bStarted || State->bCompleted)
		{
			return;
		}

		++State->MarkersTriggered;
		SpawnMarkerEnemies(Zone, Marker);
		MaybeCompleteStagedZone(Zone);
		return;
	}

	if (OrderedZones.IndexOfByKey(Zone) != CurrentZoneIndex)
	{
		return;
	}

	++MarkersTriggered;
	SpawnMarkerEnemies(Zone, Marker);

	// A marker with no valid enemies still counts as triggered; if that empties the zone, complete it.
	MaybeCompleteZone();
}

void AGP_GameMode::SpawnMarkerEnemies(AGP_EnemySpawnVolume* Zone, AGP_EnemySpawnMarker* Marker)
{
	UWorld* World = GetWorld();
	if (!World || !IsValid(Zone) || !Marker)
	{
		return;
	}

	for (const FGP_EnemySpawnEntry& Entry : Marker->GetSpawns())
	{
		if (!*Entry.EnemyClass)
		{
			continue;
		}

		for (int32 SpawnIndex = 0; SpawnIndex < Entry.Count; ++SpawnIndex)
		{
			bool bProjected = false;
			const FVector SpawnLocation = Zone->GetSpawnPointNearMarker(Marker, bProjected);
			if (!bProjected)
			{
				UE_LOG(LogTemp, Warning, TEXT("[GP_GameMode] Skipped marker spawn in zone '%s': no navmesh near marker. Check NavMeshBoundsVolume coverage."), *Zone->GetName());
				continue;
			}

			FActorSpawnParameters SpawnParameters;
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			AGP_EnemyCharacter* SpawnedEnemy = World->SpawnActor<AGP_EnemyCharacter>(
				Entry.EnemyClass, SpawnLocation, FRotator::ZeroRotator, SpawnParameters);

			if (!IsValid(SpawnedEnemy))
			{
				continue;
			}

			SpawnedEnemy->SpawnDefaultController();
			RegisterZoneEnemy(SpawnedEnemy, Zone);
		}
	}
}

void AGP_GameMode::MaybeCompleteZone()
{
	// In marker mode the zone clears only once every marker has fired and nothing is left alive.
	const bool bAllMarkersDone = (MarkersTotal == 0) || (MarkersTriggered >= MarkersTotal);
	if (bAllMarkersDone && AliveZoneEnemies == 0)
	{
		AGP_EnemySpawnVolume* Zone =
			OrderedZones.IsValidIndex(CurrentZoneIndex) ? OrderedZones[CurrentZoneIndex] : nullptr;
		if (IsValid(Zone) && Zone->HasBossPhase() && !bCurrentZoneBossPhaseStarted)
		{
			bCurrentZoneBossPhaseStarted = true;
			if (SpawnZoneBossEnemies(Zone) > 0)
			{
				return;
			}

			UE_LOG(LogTemp, Error,
				TEXT("[GP_GameMode] Zone '%s' boss phase had no successful spawns; completing zone to avoid a soft lock."),
				*Zone->GetZoneId().ToString());
		}
		CompleteCurrentZone();
	}
}

void AGP_GameMode::SpawnZoneEnemies(AGP_EnemySpawnVolume* Zone)
{
	UWorld* World = GetWorld();
	if (!IsValid(Zone) || !World)
	{
		return;
	}

	const bool bRandomize = !Zone->IsBossZone();

	for (const FGP_EnemySpawnEntry& Entry : Zone->GetSpawns())
	{
		if (!*Entry.EnemyClass)
		{
			continue;
		}

		for (int32 SpawnIndex = 0; SpawnIndex < Entry.Count; ++SpawnIndex)
		{
			bool bProjected = false;
			const FVector SpawnLocation = Zone->GetSpawnPoint(bRandomize, bProjected);
			if (!bProjected)
			{
				UE_LOG(LogTemp, Warning, TEXT("[GP_GameMode] Skipped spawn in zone '%s': no navmesh near spawn point. Check NavMeshBoundsVolume coverage."), *Zone->GetName());
				continue;
			}

			FActorSpawnParameters SpawnParameters;
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			AGP_EnemyCharacter* SpawnedEnemy = World->SpawnActor<AGP_EnemyCharacter>(
				Entry.EnemyClass,
				SpawnLocation,
				FRotator::ZeroRotator,
				SpawnParameters);

			if (!IsValid(SpawnedEnemy))
			{
				continue;
			}

			SpawnedEnemy->SpawnDefaultController();
			RegisterZoneEnemy(SpawnedEnemy, Zone);
		}
	}
}

int32 AGP_GameMode::SpawnZoneBossEnemies(AGP_EnemySpawnVolume* Zone)
{
	UWorld* World = GetWorld();
	if (!IsValid(Zone) || !World)
	{
		return 0;
	}

	int32 SpawnedCount = 0;
	for (const FGP_EnemySpawnEntry& Entry : Zone->GetBossSpawns())
	{
		if (!*Entry.EnemyClass)
		{
			continue;
		}

		for (int32 SpawnIndex = 0; SpawnIndex < Entry.Count; ++SpawnIndex)
		{
			bool bProjected = false;
			const FVector SpawnLocation = Zone->GetBossSpawnPoint(bProjected);
			if (!bProjected)
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[GP_GameMode] Skipped boss spawn in zone '%s': no navmesh at BossSpawnPoint."),
					*Zone->GetName());
				continue;
			}

			FActorSpawnParameters SpawnParameters;
			SpawnParameters.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			if (AGP_EnemyCharacter* Boss = World->SpawnActor<AGP_EnemyCharacter>(
				Entry.EnemyClass,
				SpawnLocation,
				FRotator::ZeroRotator,
				SpawnParameters))
			{
				Boss->SpawnDefaultController();
				RegisterZoneEnemy(Boss, Zone);
				++SpawnedCount;
			}
		}
	}

	UE_LOG(LogTemp, Log,
		TEXT("[GP_GameMode] Zone '%s' started boss phase with %d enemy(s)."),
		*Zone->GetZoneId().ToString(),
		SpawnedCount);
	return SpawnedCount;
}

void AGP_GameMode::RegisterZoneEnemy(
	AGP_EnemyCharacter* Enemy,
	AGP_EnemySpawnVolume* OwningZone)
{
	if (bRunFinished || !IsValid(Enemy))
	{
		return;
	}

	if (Enemy->OnEnemyDeathStarted.IsAlreadyBound(this, &AGP_GameMode::HandleZoneEnemyDied))
	{
		return;
	}

	if (bUseStagedZoneProgression && IsValid(OwningZone))
	{
		FGPZoneRuntimeState& State = ZoneRuntimeStates.FindOrAdd(OwningZone);
		State.Zone = OwningZone;
		++State.AliveEnemies;
		EnemyOwningZones.Add(Enemy, OwningZone);
	}
	else
	{
		++AliveZoneEnemies;
	}
	// The terminal death delegate also fires for scripted encounter cleanup, unlike the HP-zero reward delegate.
	Enemy->OnEnemyDeathStarted.AddDynamic(this, &AGP_GameMode::HandleZoneEnemyDied);

	if (AGP_GameState* GPGameState = GetGPGameState())
	{
		GPGameState->SetEnemiesRemaining(
			bUseStagedZoneProgression ? GetTotalAliveZoneEnemies() : AliveZoneEnemies);
	}
}

void AGP_GameMode::HandleZoneEnemyDied(AGP_EnemyCharacter* DeadEnemy, AActor* DeathInstigator)
{
	if (bUseStagedZoneProgression)
	{
		TWeakObjectPtr<AGP_EnemySpawnVolume> OwningZone;
		if (EnemyOwningZones.RemoveAndCopyValue(DeadEnemy, OwningZone))
		{
			if (FGPZoneRuntimeState* State = ZoneRuntimeStates.Find(OwningZone))
			{
				State->AliveEnemies = FMath::Max(0, State->AliveEnemies - 1);
				if (IsValid(DeadEnemy))
				{
					State->LastEnemyDeathLocation = DeadEnemy->GetActorLocation();
					State->bHasLastEnemyDeathLocation = true;
				}
			}

			if (AGP_GameState* GPGameState = GetGPGameState())
			{
				GPGameState->SetEnemiesRemaining(GetTotalAliveZoneEnemies());
			}
			MaybeCompleteStagedZone(OwningZone.Get());
		}
		return;
	}

	AliveZoneEnemies = FMath::Max(0, AliveZoneEnemies - 1);

	// Remember where this enemy fell so the advance portal can spawn on the kill spot.
	if (IsValid(DeadEnemy))
	{
		LastEnemyDeathLocation = DeadEnemy->GetActorLocation();
		bHasLastDeathLocation = true;
	}

	if (AGP_GameState* GPGameState = GetGPGameState())
	{
		GPGameState->SetEnemiesRemaining(AliveZoneEnemies);
	}

	MaybeCompleteZone();
}

int32 AGP_GameMode::GetTotalAliveZoneEnemies() const
{
	int32 Total = 0;
	for (const TPair<TWeakObjectPtr<AGP_EnemySpawnVolume>, FGPZoneRuntimeState>& Pair : ZoneRuntimeStates)
	{
		if (!Pair.Value.bCompleted)
		{
			Total += Pair.Value.AliveEnemies;
		}
	}
	return Total;
}
