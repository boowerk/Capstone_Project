#pragma once

#include "CoreMinimal.h"
#include "Game/WorldLayout/GP_VillageTypes.h"

namespace GPVillagePresetPolicy
{
	PROJECT_EDEN_API int32 SelectPresetIndex(
		int32 RunSeed,
		FName SlotId,
		const TArray<FGP_VillagePresetDefinition>& Presets,
		int32 AssignmentAttempt = 0,
		EGP_VillageSlotSizeClass SlotSizeClass = EGP_VillageSlotSizeClass::Medium);

	PROJECT_EDEN_API TArray<FGP_VillagePresetDefinition> MergePresetSources(
		const TArray<FGP_VillagePresetDefinition>& PrimaryPresets,
		const TArray<FGP_VillagePresetDefinition>& CatalogPresets,
		const TArray<FGP_VillagePresetDefinition>& LegacyPresets);

	PROJECT_EDEN_API bool DoesFootprintFitSlot(
		const FGP_VillageFootprint& Footprint,
		EGP_VillageSlotSizeClass SlotSizeClass);

	PROJECT_EDEN_API bool DoFootprintsOverlap(
		const FTransform& FirstSlotTransform,
		const FGP_VillageFootprint& FirstFootprint,
		const FTransform& SecondSlotTransform,
		const FGP_VillageFootprint& SecondFootprint);
}
