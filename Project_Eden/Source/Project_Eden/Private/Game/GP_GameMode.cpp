#include "Game/GP_GameMode.h"

#include "Characters/GP_EnemyCharacter.h"
#include "Game/GP_EnemySpawnVolume.h"
#include "Game/GP_GameState.h"
#include "Kismet/GameplayStatics.h"

AGP_GameMode::AGP_GameMode()
{
	GameStateClass = AGP_GameState::StaticClass();
}

void AGP_GameMode::BeginPlay()
{
	Super::BeginPlay();

	GatherZones();

	// Delay the first spawn so navmesh/streaming has time to be ready; StartRun can also be called from BP.
	if (StartDelaySeconds > 0.0f)
	{
		FTimerHandle StartTimerHandle;
		GetWorldTimerManager().SetTimer(StartTimerHandle, this, &AGP_GameMode::StartRun, StartDelaySeconds, false);
	}
	else
	{
		StartRun();
	}
}

void AGP_GameMode::GatherZones()
{
	TArray<AActor*> FoundZones;
	UGameplayStatics::GetAllActorsOfClass(this, AGP_EnemySpawnVolume::StaticClass(), FoundZones);

	OrderedZones.Reset();
	for (AActor* FoundZone : FoundZones)
	{
		if (AGP_EnemySpawnVolume* Zone = Cast<AGP_EnemySpawnVolume>(FoundZone))
		{
			OrderedZones.Add(Zone);
		}
	}

	OrderedZones.Sort([](const AGP_EnemySpawnVolume& A, const AGP_EnemySpawnVolume& B)
	{
		return A.GetZoneOrder() < B.GetZoneOrder();
	});
}

void AGP_GameMode::StartRun()
{
	if (bRunStarted)
	{
		return;
	}

	bRunStarted = true;

	if (OrderedZones.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GP_GameMode] No AGP_EnemySpawnVolume zones found in the level; nothing to run."));
		FinishRun(/*bVictory=*/true);
		return;
	}

	StartZone(0);
}

void AGP_GameMode::StartZone(int32 ZoneIndex)
{
	if (bRunFinished)
	{
		return;
	}

	if (!OrderedZones.IsValidIndex(ZoneIndex))
	{
		// Ran past the end of the sequence: that is a victory.
		FinishRun(/*bVictory=*/true);
		return;
	}

	CurrentZoneIndex = ZoneIndex;
	AliveZoneEnemies = 0;

	AGP_EnemySpawnVolume* Zone = OrderedZones[ZoneIndex];

	if (AGP_GameState* GPGameState = GetGPGameState())
	{
		GPGameState->SetCurrentZoneIndex(ZoneIndex);
		GPGameState->SetEnemiesRemaining(0);
		GPGameState->SetMatchPhase(Zone->IsBossZone() ? EGPMatchPhase::BossFight : EGPMatchPhase::InCity);
	}

	SpawnZoneEnemies(Zone);

	// Extra presentation/setup after the zone's enemies exist.
	OnZoneStarted(ZoneIndex, Zone);

	// A zone with no valid spawns would otherwise stall the run; treat it as immediately cleared.
	if (AliveZoneEnemies == 0)
	{
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
				// No navmesh under the chosen point; skip rather than drop an enemy that cannot path.
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

			// Runtime-spawned enemies still need their AI controller; placed enemies auto-possess on their own.
			SpawnedEnemy->SpawnDefaultController();
			RegisterZoneEnemy(SpawnedEnemy);
		}
	}
}

void AGP_GameMode::RegisterZoneEnemy(AGP_EnemyCharacter* Enemy)
{
	if (bRunFinished || !IsValid(Enemy))
	{
		return;
	}

	// Avoid double-counting if the same enemy is registered twice.
	if (Enemy->OnEnemyDied.IsAlreadyBound(this, &AGP_GameMode::HandleZoneEnemyDied))
	{
		return;
	}

	Enemy->OnEnemyDied.AddDynamic(this, &AGP_GameMode::HandleZoneEnemyDied);
	++AliveZoneEnemies;

	if (AGP_GameState* GPGameState = GetGPGameState())
	{
		GPGameState->SetEnemiesRemaining(AliveZoneEnemies);
	}
}

void AGP_GameMode::HandleZoneEnemyDied(AGP_EnemyCharacter* DeadEnemy)
{
	AliveZoneEnemies = FMath::Max(0, AliveZoneEnemies - 1);

	if (AGP_GameState* GPGameState = GetGPGameState())
	{
		GPGameState->SetEnemiesRemaining(AliveZoneEnemies);
	}

	if (AliveZoneEnemies == 0)
	{
		CompleteCurrentZone();
	}
}

void AGP_GameMode::CompleteCurrentZone()
{
	if (bRunFinished || !OrderedZones.IsValidIndex(CurrentZoneIndex))
	{
		return;
	}

	// Cleared-city vegetation change and the boss-room teleport/gate are wired in the Blueprint event.
	OnZoneCompleted(CurrentZoneIndex, OrderedZones[CurrentZoneIndex]);

	AdvanceZone();
}

void AGP_GameMode::AdvanceZone()
{
	const int32 NextZoneIndex = CurrentZoneIndex + 1;

	if (!OrderedZones.IsValidIndex(NextZoneIndex))
	{
		FinishRun(/*bVictory=*/true);
		return;
	}

	if (AGP_GameState* GPGameState = GetGPGameState())
	{
		GPGameState->SetMatchPhase(EGPMatchPhase::Transition);
	}

	StartZone(NextZoneIndex);
}

void AGP_GameMode::NotifyAllPlayersDead()
{
	FinishRun(/*bVictory=*/false);
}

void AGP_GameMode::FinishRun(bool bVictory)
{
	if (bRunFinished)
	{
		return;
	}

	bRunFinished = true;

	if (AGP_GameState* GPGameState = GetGPGameState())
	{
		GPGameState->SetMatchPhase(bVictory ? EGPMatchPhase::Victory : EGPMatchPhase::Defeat);
	}

	OnRunFinished(bVictory);
}

AGP_GameState* AGP_GameMode::GetGPGameState() const
{
	return GetGameState<AGP_GameState>();
}
