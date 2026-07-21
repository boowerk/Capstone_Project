#pragma once

#include "CoreMinimal.h"
#include "Game/WorldLayout/GP_VillageTypes.h"

namespace GPVillageSelectionPolicy
{
	PROJECT_EDEN_API FGP_VillageSelectionResult SelectSlots(
		int32 RunSeed,
		const TArray<FGP_VillageCandidate>& Candidates,
		const TArray<FGP_VillageGroupRule>& GroupRules);
}
