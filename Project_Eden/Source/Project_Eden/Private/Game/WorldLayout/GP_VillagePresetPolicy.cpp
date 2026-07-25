#include "Game/WorldLayout/GP_VillagePresetPolicy.h"

#include "Misc/Crc.h"

namespace
{
	struct FGPVillageFootprint2D
	{
		FVector2D Center = FVector2D::ZeroVector;
		FVector2D AxisX = FVector2D(1.0f, 0.0f);
		FVector2D AxisY = FVector2D(0.0f, 1.0f);
		FVector2D Extent = FVector2D::ZeroVector;
	};

	FGPVillageFootprint2D MakeFootprint2D(
		const FTransform& SlotTransform,
		const FGP_VillageFootprint& Footprint)
	{
		const FTransform UnitScaleTransform(
			SlotTransform.GetRotation(),
			SlotTransform.GetLocation(),
			FVector::OneVector);
		const FVector WorldCenter = UnitScaleTransform.TransformPosition(Footprint.FootprintOffset);
		const float YawRadians = FMath::DegreesToRadians(UnitScaleTransform.Rotator().Yaw);

		FGPVillageFootprint2D Result;
		Result.Center = FVector2D(WorldCenter.X, WorldCenter.Y);
		Result.AxisX = FVector2D(FMath::Cos(YawRadians), FMath::Sin(YawRadians));
		Result.AxisY = FVector2D(-Result.AxisX.Y, Result.AxisX.X);
		Result.Extent = FVector2D(
			FMath::Max(FMath::Abs(Footprint.FootprintExtent.X), 1.0f),
			FMath::Max(FMath::Abs(Footprint.FootprintExtent.Y), 1.0f));
		return Result;
	}

	float ProjectedRadius(const FGPVillageFootprint2D& Footprint, const FVector2D& Axis)
	{
		return Footprint.Extent.X * FMath::Abs(FVector2D::DotProduct(Footprint.AxisX, Axis))
			+ Footprint.Extent.Y * FMath::Abs(FVector2D::DotProduct(Footprint.AxisY, Axis));
	}

	FString MakePresetSortKey(const FGP_VillagePresetDefinition& Preset)
	{
		return FString::Printf(
			TEXT("%s|%s"),
			*Preset.PresetId.ToString(),
			*Preset.VillageLevel.ToSoftObjectPath().ToString());
	}

	FName ResolvePresetId(const FGP_VillagePresetDefinition& Preset)
	{
		return Preset.PresetId.IsNone()
			? FName(*Preset.VillageLevel.ToSoftObjectPath().GetAssetName())
			: Preset.PresetId;
	}
}

int32 GPVillagePresetPolicy::SelectPresetIndex(
	int32 RunSeed,
	FName SlotId,
	const TArray<FGP_VillagePresetDefinition>& Presets,
	int32 AssignmentAttempt,
	EGP_VillageSlotSizeClass SlotSizeClass)
{
	TMap<FName, int32> PresetIdCounts;
	TMap<FString, int32> LevelPathCounts;
	for (const FGP_VillagePresetDefinition& Preset : Presets)
	{
		if (Preset.VillageLevel.IsNull() || Preset.SelectionWeight <= 0.0f)
		{
			continue;
		}

		++PresetIdCounts.FindOrAdd(ResolvePresetId(Preset));
		++LevelPathCounts.FindOrAdd(Preset.VillageLevel.ToSoftObjectPath().ToString());
	}

	TArray<int32> ValidPresetIndices;
	for (int32 PresetIndex = 0; PresetIndex < Presets.Num(); ++PresetIndex)
	{
		const FGP_VillagePresetDefinition& Preset = Presets[PresetIndex];
		const FName PresetId = ResolvePresetId(Preset);
		const FString LevelPath = Preset.VillageLevel.ToSoftObjectPath().ToString();
		if (!Preset.VillageLevel.IsNull()
			&& Preset.SelectionWeight > 0.0f
			&& PresetIdCounts.FindRef(PresetId) == 1
			&& LevelPathCounts.FindRef(LevelPath) == 1
			&& Preset.PresetSizeClass == SlotSizeClass
			&& DoesFootprintFitSlot(Preset.Footprint, SlotSizeClass))
		{
			ValidPresetIndices.Add(PresetIndex);
		}
	}

	ValidPresetIndices.Sort([&Presets](int32 FirstIndex, int32 SecondIndex)
	{
		return MakePresetSortKey(Presets[FirstIndex]) < MakePresetSortKey(Presets[SecondIndex]);
	});
	if (ValidPresetIndices.IsEmpty())
	{
		return INDEX_NONE;
	}

	float TotalWeight = 0.0f;
	for (int32 PresetIndex : ValidPresetIndices)
	{
		TotalWeight += Presets[PresetIndex].SelectionWeight;
	}

	const uint32 SlotHash = FCrc::StrCrc32(*SlotId.ToString());
	FRandomStream RandomStream(static_cast<int32>(
		HashCombineFast(
			HashCombineFast(static_cast<uint32>(RunSeed), SlotHash),
			HashCombineFast(0xA511E9B3u, static_cast<uint32>(AssignmentAttempt)))));
	float Roll = RandomStream.FRandRange(0.0f, TotalWeight);
	for (int32 PresetIndex : ValidPresetIndices)
	{
		Roll -= Presets[PresetIndex].SelectionWeight;
		if (Roll <= 0.0f)
		{
			return PresetIndex;
		}
	}

	return ValidPresetIndices.Last();
}

TArray<FGP_VillagePresetDefinition> GPVillagePresetPolicy::MergePresetSources(
	const TArray<FGP_VillagePresetDefinition>& PrimaryPresets,
	const TArray<FGP_VillagePresetDefinition>& CatalogPresets,
	const TArray<FGP_VillagePresetDefinition>& LegacyPresets)
{
	TArray<FGP_VillagePresetDefinition> EffectivePresets;
	TSet<FName> SeenPresetIds;
	TSet<FString> SeenLevelPaths;

	const auto AddPreset =
		[&EffectivePresets, &SeenPresetIds, &SeenLevelPaths](
			const FGP_VillagePresetDefinition& Preset,
			bool bReserveDisabledIdentity)
		{
			if (Preset.VillageLevel.IsNull())
			{
				return;
			}

			const FName PresetId = ResolvePresetId(Preset);
			const FString LevelPath = Preset.VillageLevel.ToSoftObjectPath().ToString();
			if (SeenPresetIds.Contains(PresetId)
				|| SeenLevelPaths.Contains(LevelPath))
			{
				return;
			}

			if (bReserveDisabledIdentity)
			{
				SeenPresetIds.Add(PresetId);
				SeenLevelPaths.Add(LevelPath);
			}

			if (Preset.SelectionWeight <= 0.0f)
			{
				return;
			}

			EffectivePresets.Add(Preset);
			if (!bReserveDisabledIdentity)
			{
				SeenPresetIds.Add(PresetId);
				SeenLevelPaths.Add(LevelPath);
			}
		};

	for (const FGP_VillagePresetDefinition& Preset : PrimaryPresets)
	{
		AddPreset(Preset, false);
	}
	for (const FGP_VillagePresetDefinition& Preset : CatalogPresets)
	{
		// A disabled catalog row is a tombstone for matching legacy entries.
		AddPreset(Preset, true);
	}
	for (const FGP_VillagePresetDefinition& Preset : LegacyPresets)
	{
		AddPreset(Preset, false);
	}
	return EffectivePresets;
}

bool GPVillagePresetPolicy::DoesFootprintFitSlot(
	const FGP_VillageFootprint& Footprint,
	EGP_VillageSlotSizeClass SlotSizeClass)
{
	const FVector2D CapacityHalfExtent = GPGetVillageSlotCapacityHalfExtent(SlotSizeClass);
	const FVector2D RequiredHalfExtent(
		FMath::Abs(Footprint.FootprintOffset.X)
			+ FMath::Abs(Footprint.FootprintExtent.X),
		FMath::Abs(Footprint.FootprintOffset.Y)
			+ FMath::Abs(Footprint.FootprintExtent.Y));
	return RequiredHalfExtent.X <= CapacityHalfExtent.X + KINDA_SMALL_NUMBER
		&& RequiredHalfExtent.Y <= CapacityHalfExtent.Y + KINDA_SMALL_NUMBER;
}

bool GPVillagePresetPolicy::DoFootprintsOverlap(
	const FTransform& FirstSlotTransform,
	const FGP_VillageFootprint& FirstFootprint,
	const FTransform& SecondSlotTransform,
	const FGP_VillageFootprint& SecondFootprint)
{
	const FGPVillageFootprint2D First = MakeFootprint2D(FirstSlotTransform, FirstFootprint);
	const FGPVillageFootprint2D Second = MakeFootprint2D(SecondSlotTransform, SecondFootprint);
	const FVector2D CenterDelta = Second.Center - First.Center;
	const FVector2D Axes[] = {First.AxisX, First.AxisY, Second.AxisX, Second.AxisY};

	for (const FVector2D& Axis : Axes)
	{
		const float CenterDistance = FMath::Abs(FVector2D::DotProduct(CenterDelta, Axis));
		const float CombinedRadius = ProjectedRadius(First, Axis) + ProjectedRadius(Second, Axis);
		if (CenterDistance >= CombinedRadius - KINDA_SMALL_NUMBER)
		{
			return false;
		}
	}

	return true;
}
