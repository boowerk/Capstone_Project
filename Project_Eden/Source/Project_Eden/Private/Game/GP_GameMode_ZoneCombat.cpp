#include "Game/GP_GameMode.h"

#include "Characters/GP_EnemyCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Game/GP_EnemySpawnMarker.h"
#include "Game/GP_EnemySpawnVolume.h"
#include "Game/GP_GameState.h"
#include "TimerManager.h"

namespace GPZoneCombat
{
	constexpr int32 ImmediatePlacementAttempts = 8;

	FVector GetEnemyGroundLocation(const AGP_EnemyCharacter& Enemy)
	{
		FVector GroundLocation = Enemy.GetActorLocation();
		if (const UCapsuleComponent* Capsule = Enemy.GetCapsuleComponent())
		{
			GroundLocation.Z -= Capsule->GetScaledCapsuleHalfHeight();
		}
		return GroundLocation;
	}

	bool IsEnemyPlacementValid(
		const AGP_EnemySpawnVolume& Zone,
		const FVector& RequestedGroundLocation,
		const AGP_EnemyCharacter& Enemy)
	{
		return Zone.IsFinalSpawnGroundLocationValid(
			RequestedGroundLocation,
			GetEnemyGroundLocation(Enemy));
	}
}

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
			AGP_EnemyCharacter* SpawnedEnemy = nullptr;
			for (int32 AttemptIndex = 0;
				AttemptIndex < GPZoneCombat::ImmediatePlacementAttempts;
				++AttemptIndex)
			{
				bool bProjected = false;
				const FVector SpawnLocation =
					Zone->GetSpawnPointNearMarker(Marker, bProjected);
				if (!bProjected)
				{
					continue;
				}

				FActorSpawnParameters SpawnParameters;
				SpawnParameters.SpawnCollisionHandlingOverride =
					ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

				SpawnedEnemy = World->SpawnActor<AGP_EnemyCharacter>(
					Entry.EnemyClass,
					SpawnLocation,
					FRotator::ZeroRotator,
					SpawnParameters);
				if (!IsValid(SpawnedEnemy))
				{
					continue;
				}

				if (!GPZoneCombat::IsEnemyPlacementValid(
					*Zone,
					SpawnLocation,
					*SpawnedEnemy))
				{
					SpawnedEnemy->Destroy();
					SpawnedEnemy = nullptr;
					continue;
				}

				break;
			}

			if (!IsValid(SpawnedEnemy))
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[GP_GameMode] Skipped marker spawn in zone '%s': no safe ground placement was available."),
					*Zone->GetName());
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
	if (bAllMarkersDone
		&& AliveZoneEnemies == 0
		&& PendingZoneEnemySpawns == 0)
	{
		AGP_EnemySpawnVolume* Zone =
			OrderedZones.IsValidIndex(CurrentZoneIndex) ? OrderedZones[CurrentZoneIndex] : nullptr;
		if (IsValid(Zone) && Zone->HasBossPhase() && !bCurrentZoneBossPhaseStarted)
		{
			if (bCurrentZoneBossSpawnRetryPending)
			{
				return;
			}

			bCurrentZoneBossPhaseStarted = true;
			const int32 BossSpawnCount = SpawnZoneBossEnemies(Zone);
			if (BossSpawnCount > 0)
			{
				CurrentZoneBossSpawnRetryCount = 0;
				return;
			}
			if (BossSpawnCount == INDEX_NONE)
			{
				// Invalid content must remain visibly blocked for authoring
				// instead of being mislabeled as a transient navigation retry.
				return;
			}

			bCurrentZoneBossPhaseStarted = false;
			ScheduleZoneBossSpawnRetry(Zone);
			return;
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

		SpawnZoneEnemyBatch(
			Zone,
			Entry.EnemyClass,
			Entry.Count,
			bRandomize,
			/*RetryCount=*/0);
	}
}

void AGP_GameMode::SpawnZoneEnemyBatch(
	AGP_EnemySpawnVolume* Zone,
	TSubclassOf<AGP_EnemyCharacter> EnemyClass,
	int32 SpawnCount,
	bool bRandomize,
	int32 RetryCount)
{
	UWorld* World = GetWorld();
	if (bRunFinished
		|| !IsValid(Zone)
		|| !World
		|| !*EnemyClass
		|| SpawnCount <= 0)
	{
		return;
	}

	int32* PendingSpawnCount = nullptr;
	if (bUseStagedZoneProgression)
	{
		FGPZoneRuntimeState* State = ZoneRuntimeStates.Find(Zone);
		if (!State || !State->bStarted || State->bCompleted)
		{
			return;
		}
		PendingSpawnCount = &State->PendingEnemySpawns;
	}
	else
	{
		if (!OrderedZones.IsValidIndex(CurrentZoneIndex)
			|| OrderedZones[CurrentZoneIndex] != Zone)
		{
			return;
		}
		PendingSpawnCount = &PendingZoneEnemySpawns;
	}

	if (RetryCount == 0)
	{
		*PendingSpawnCount += SpawnCount;
	}

	int32 PlacementFailures = 0;
	int32 ResolvedSpawnCount = 0;
	for (int32 SpawnIndex = 0; SpawnIndex < SpawnCount; ++SpawnIndex)
	{
		bool bProjected = false;
		const FVector SpawnLocation = Zone->GetSpawnPoint(bRandomize, bProjected);
		if (!bProjected)
		{
			++PlacementFailures;
			continue;
		}

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		AGP_EnemyCharacter* SpawnedEnemy = World->SpawnActor<AGP_EnemyCharacter>(
			EnemyClass,
			SpawnLocation,
			FRotator::ZeroRotator,
			SpawnParameters);
		if (!IsValid(SpawnedEnemy))
		{
			++PlacementFailures;
			continue;
		}

		if (!GPZoneCombat::IsEnemyPlacementValid(
			*Zone,
			SpawnLocation,
			*SpawnedEnemy))
		{
			SpawnedEnemy->Destroy();
			++PlacementFailures;
			continue;
		}

		++ResolvedSpawnCount;
		SpawnedEnemy->SpawnDefaultController();
		RegisterZoneEnemy(SpawnedEnemy, Zone);
	}

	const float RetryInterval = FMath::Max(0.05f, ZoneNavigationRetryInterval);
	const int32 MaxRetries = FMath::Max(
		1,
		FMath::CeilToInt(ZoneNavigationReadyTimeout / RetryInterval));
	const bool bCanRetryPlacement =
		PlacementFailures > 0 && RetryCount < MaxRetries;

	if (!bCanRetryPlacement)
	{
		ResolvedSpawnCount += PlacementFailures;
		if (PlacementFailures > 0)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[GP_GameMode] Zone '%s' abandoned %d enemy spawn(s): no safe navigation/collision placement was available for %.1f seconds."),
				*Zone->GetZoneId().ToString(),
				PlacementFailures,
				ZoneNavigationReadyTimeout);
		}
	}

	*PendingSpawnCount = FMath::Max(0, *PendingSpawnCount - ResolvedSpawnCount);

	if (bCanRetryPlacement)
	{
		const TWeakObjectPtr<AGP_EnemySpawnVolume> WeakZone = Zone;
		FTimerHandle RetryTimer;
		World->GetTimerManager().SetTimer(
			RetryTimer,
			FTimerDelegate::CreateWeakLambda(
				this,
				[this, WeakZone, EnemyClass, PlacementFailures, bRandomize, RetryCount]()
				{
					if (WeakZone.IsValid())
					{
						SpawnZoneEnemyBatch(
							WeakZone.Get(),
							EnemyClass,
							PlacementFailures,
							bRandomize,
							RetryCount + 1);
					}
				}),
			RetryInterval,
			false);
	}

	// Initial callers finish adding every composition entry before performing
	// their completion check. Retry callbacks must re-evaluate the zone because
	// all already-spawned enemies may have died while navigation was building.
	if (RetryCount > 0)
	{
		if (bUseStagedZoneProgression)
		{
			MaybeCompleteStagedZone(Zone);
		}
		else
		{
			MaybeCompleteZone();
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

	TArray<AGP_EnemyCharacter*> PendingBosses;
	bool bPlacementFailed = false;
	bool bHasValidConfiguration = false;
	for (const FGP_EnemySpawnEntry& Entry : Zone->GetBossSpawns())
	{
		if (!*Entry.EnemyClass || Entry.Count <= 0)
		{
			continue;
		}
		bHasValidConfiguration = true;

		for (int32 SpawnIndex = 0; SpawnIndex < Entry.Count; ++SpawnIndex)
		{
			bool bProjected = false;
			const FVector SpawnLocation = Zone->GetBossSpawnPoint(bProjected);
			if (!bProjected)
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[GP_GameMode] Skipped boss spawn in zone '%s': no navmesh at BossSpawnPoint."),
					*Zone->GetName());
				bPlacementFailed = true;
				continue;
			}

			FActorSpawnParameters SpawnParameters;
			SpawnParameters.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			AGP_EnemyCharacter* Boss = World->SpawnActor<AGP_EnemyCharacter>(
				Entry.EnemyClass,
				SpawnLocation,
				FRotator::ZeroRotator,
				SpawnParameters);
			if (IsValid(Boss)
				&& GPZoneCombat::IsEnemyPlacementValid(
					*Zone,
					SpawnLocation,
					*Boss))
			{
				PendingBosses.Add(Boss);
			}
			else
			{
				if (IsValid(Boss))
				{
					Boss->Destroy();
				}
				bPlacementFailed = true;
				UE_LOG(LogTemp, Error,
					TEXT("[GP_GameMode] Failed to place boss class '%s' on safe ground in zone '%s'; the spawn will be retried."),
					*GetNameSafe(*Entry.EnemyClass),
					*Zone->GetZoneId().ToString());
			}
		}
	}

	if (!bHasValidConfiguration)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[GP_GameMode] Zone '%s' has a boss phase but no valid boss class/count configuration; progression remains blocked."),
			*Zone->GetZoneId().ToString());
		return INDEX_NONE;
	}

	if (bPlacementFailed)
	{
		for (AGP_EnemyCharacter* PendingBoss : PendingBosses)
		{
			if (IsValid(PendingBoss))
			{
				PendingBoss->Destroy();
			}
		}
		return 0;
	}

	for (AGP_EnemyCharacter* Boss : PendingBosses)
	{
		Boss->SpawnDefaultController();
		RegisterZoneEnemy(Boss, Zone);
	}

	const int32 SpawnedCount = PendingBosses.Num();
	UE_LOG(LogTemp, Log,
		TEXT("[GP_GameMode] Zone '%s' started boss phase with %d enemy(s)."),
		*Zone->GetZoneId().ToString(),
		SpawnedCount);
	return SpawnedCount;
}

void AGP_GameMode::ScheduleZoneBossSpawnRetry(AGP_EnemySpawnVolume* Zone)
{
	UWorld* World = GetWorld();
	if (bRunFinished || !IsValid(Zone) || !IsValid(World))
	{
		return;
	}

	int32* RetryCount = nullptr;
	bool* bRetryPending = nullptr;
	if (bUseStagedZoneProgression)
	{
		FGPZoneRuntimeState* State = ZoneRuntimeStates.Find(Zone);
		if (!State || !State->bStarted || State->bCompleted)
		{
			return;
		}
		RetryCount = &State->BossSpawnRetryCount;
		bRetryPending = &State->bBossSpawnRetryPending;
	}
	else
	{
		if (!OrderedZones.IsValidIndex(CurrentZoneIndex)
			|| OrderedZones[CurrentZoneIndex] != Zone)
		{
			return;
		}
		RetryCount = &CurrentZoneBossSpawnRetryCount;
		bRetryPending = &bCurrentZoneBossSpawnRetryPending;
	}

	if (*bRetryPending)
	{
		return;
	}

	*bRetryPending = true;
	++(*RetryCount);

	const float RetryInterval = FMath::Max(0.05f, ZoneNavigationRetryInterval);
	const int32 WarningRetryCount = FMath::Max(
		1,
		FMath::CeilToInt(ZoneNavigationReadyTimeout / RetryInterval));
	if (*RetryCount == 1)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[GP_GameMode] Zone '%s' boss spawn failed; waiting for a valid spawn surface/runtime actor and keeping progression blocked."),
			*Zone->GetZoneId().ToString());
	}
	else if (*RetryCount == WarningRetryCount)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[GP_GameMode] Zone '%s' boss still cannot spawn after %.1f seconds; retries will continue and no portal will open."),
			*Zone->GetZoneId().ToString(),
			ZoneNavigationReadyTimeout);
	}

	const TWeakObjectPtr<AGP_EnemySpawnVolume> WeakZone = Zone;
	FTimerHandle RetryTimer;
	World->GetTimerManager().SetTimer(
		RetryTimer,
		FTimerDelegate::CreateWeakLambda(this, [this, WeakZone]()
		{
			if (!WeakZone.IsValid())
			{
				return;
			}

			AGP_EnemySpawnVolume* RetryZone = WeakZone.Get();
			if (bUseStagedZoneProgression)
			{
				if (FGPZoneRuntimeState* State = ZoneRuntimeStates.Find(RetryZone))
				{
					State->bBossSpawnRetryPending = false;
					MaybeCompleteStagedZone(RetryZone);
				}
			}
			else if (OrderedZones.IsValidIndex(CurrentZoneIndex)
				&& OrderedZones[CurrentZoneIndex] == RetryZone)
			{
				bCurrentZoneBossSpawnRetryPending = false;
				MaybeCompleteZone();
			}
		}),
		RetryInterval,
		false);
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
