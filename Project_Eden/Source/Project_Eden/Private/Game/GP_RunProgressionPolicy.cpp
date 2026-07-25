#include "Game/GP_RunProgressionPolicy.h"

#include "Game/GP_EnemySpawnVolume.h"

namespace GPRunProgressionPolicy
{
	bool IsOuterStageReady(
		int32 ActivePlayerCount,
		int32 AssignedOuterZoneCount,
		int32 CompletedAssignedOuterZoneCount)
	{
		return ActivePlayerCount > 0
			&& AssignedOuterZoneCount == ActivePlayerCount
			&& CompletedAssignedOuterZoneCount == AssignedOuterZoneCount;
	}

	bool IsPartyDefeated(
		int32 ParticipantCount,
		int32 EliminatedParticipantCount)
	{
		return ParticipantCount > 0
			&& EliminatedParticipantCount >= ParticipantCount;
	}

	bool CanCountStagedZonePresence(
		EGPZoneStage ZoneStage,
		bool bHasMatchingPortalArrival)
	{
		return ZoneStage != EGPZoneStage::Colosseum
			|| bHasMatchingPortalArrival;
	}

	bool ShouldStartColosseumIntro(
		bool bAllActivePlayersPresent,
		bool bIntroStarted,
		bool bIntroCompleted)
	{
		return bAllActivePlayersPresent
			&& !bIntroStarted
			&& !bIntroCompleted;
	}

	bool RequiresFullPartyAtEncounterStart(EGPZoneStage ZoneStage)
	{
		// Colosseum admission is frozen when its intro starts. Only Center still
		// needs a live presence check at the point the encounter itself starts.
		return ZoneStage == EGPZoneStage::Center;
	}

	bool ShouldRelocateJoiningPlayerToZone(
		EGPZoneStage ZoneStage,
		bool bIntroStarted,
		bool bIntroCompleted,
		bool bEncounterStarted,
		bool bZoneCompleted)
	{
		return ZoneStage == EGPZoneStage::Colosseum
			&& !bZoneCompleted
			&& (bIntroStarted || bIntroCompleted || bEncounterStarted);
	}
}
