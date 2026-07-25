#pragma once

#include "CoreTypes.h"

enum class EGPZoneStage : uint8;

namespace GPRunProgressionPolicy
{
	PROJECT_EDEN_API bool IsOuterStageReady(
		int32 ActivePlayerCount,
		int32 AssignedOuterZoneCount,
		int32 CompletedAssignedOuterZoneCount);

	PROJECT_EDEN_API bool IsPartyDefeated(
		int32 ParticipantCount,
		int32 EliminatedParticipantCount);

	PROJECT_EDEN_API bool CanCountStagedZonePresence(
		EGPZoneStage ZoneStage,
		bool bHasMatchingPortalArrival);

	PROJECT_EDEN_API bool ShouldStartColosseumIntro(
		bool bAllActivePlayersPresent,
		bool bIntroStarted,
		bool bIntroCompleted);

	PROJECT_EDEN_API bool RequiresFullPartyAtEncounterStart(
		EGPZoneStage ZoneStage);

	PROJECT_EDEN_API bool ShouldRelocateJoiningPlayerToZone(
		EGPZoneStage ZoneStage,
		bool bIntroStarted,
		bool bIntroCompleted,
		bool bEncounterStarted,
		bool bZoneCompleted);
}
