#pragma once

#include "CoreMinimal.h"

class AGP_EnemySpawnVolume;
class APlayerState;

struct FGPZoneRuntimeState
{
	TWeakObjectPtr<AGP_EnemySpawnVolume> Zone;
	TSet<TWeakObjectPtr<APlayerState>> Participants;
	TSet<TWeakObjectPtr<APlayerState>> PresentPlayers;
	TSet<TWeakObjectPtr<APlayerState>> PortalArrivals;
	int32 MarkersTotal = 0;
	int32 MarkersTriggered = 0;
	int32 AliveEnemies = 0;
	int32 PendingEnemySpawns = 0;
	int32 BossSpawnRetryCount = 0;
	bool bBossSpawnRetryPending = false;
	bool bBossPhaseStarted = false;
	FVector LastEnemyDeathLocation = FVector::ZeroVector;
	bool bHasLastEnemyDeathLocation = false;
	bool bIntroStarted = false;
	bool bIntroCompleted = false;
	bool bStarted = false;
	bool bCompleted = false;
};
