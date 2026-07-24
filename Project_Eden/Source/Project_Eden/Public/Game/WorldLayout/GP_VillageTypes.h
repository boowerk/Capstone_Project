#pragma once

#include "CoreMinimal.h"
#include "GP_VillageTypes.generated.h"

class UWorld;

UENUM(BlueprintType)
enum class EGP_VillageSlotSizeClass : uint8
{
	Small UMETA(DisplayName = "Small (130 m)"),
	Medium UMETA(DisplayName = "Medium (230 m)")
};

inline FVector2D GPGetVillageSlotCapacityHalfExtent(EGP_VillageSlotSizeClass SizeClass)
{
	switch (SizeClass)
	{
	case EGP_VillageSlotSizeClass::Small:
		return FVector2D(6500.0f, 6500.0f);
	case EGP_VillageSlotSizeClass::Medium:
	default:
		return FVector2D(11500.0f, 11500.0f);
	}
}

USTRUCT(BlueprintType)
struct FGP_VillageFootprint
{
	GENERATED_BODY()

	// Local-space center offset from the village slot origin.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village|Footprint", meta = (Units = "cm"))
	FVector FootprintOffset = FVector(0.0f, 0.0f, -1500.0f);

	// Half extent. The default represents a 230 m x 230 m authored village area.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village|Footprint", meta = (ClampMin = "1.0", Units = "cm"))
	FVector FootprintExtent = FVector(11500.0f, 11500.0f, 3000.0f);
};

USTRUCT(BlueprintType)
struct FGP_VillagePresetDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village|Preset")
	FName PresetId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village|Preset")
	TSoftObjectPtr<UWorld> VillageLevel;

	// A preset is eligible only for slots with the same size class. Footprint
	// capacity is still validated separately to catch incorrect authored bounds.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village|Preset")
	EGP_VillageSlotSizeClass PresetSizeClass = EGP_VillageSlotSizeClass::Medium;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village|Preset", meta = (ShowOnlyInnerProperties))
	FGP_VillageFootprint Footprint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village|Preset", meta = (ClampMin = "0.0"))
	float SelectionWeight = 1.0f;
};

// One server-selected village instance. Clients use this compact description to
// load the same level and generate its visual PCG locally; generated ISMs are not
// replicated individually.
USTRUCT()
struct FGP_VillageRuntimeInstance
{
	GENERATED_BODY()

	UPROPERTY()
	FName SlotId = NAME_None;

	UPROPERTY()
	FName PresetId = NAME_None;

	UPROPERTY()
	TSoftObjectPtr<UWorld> VillageLevel;

	UPROPERTY()
	FTransform SpawnTransform = FTransform::Identity;

	UPROPERTY()
	FName StableInstanceName = NAME_None;

	UPROPERTY()
	int32 PCGSeed = INDEX_NONE;
};

// Replicated atomically so late-joining clients receive the exact server choice
// instead of recomputing it from a potentially different catalog.
USTRUCT()
struct FGP_VillageRuntimeLayoutSnapshot
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Revision = 0;

	UPROPERTY()
	int32 RunSeed = INDEX_NONE;

	// True means Instances is a complete server-approved layout. False tells
	// clients to unload any previously applied layout (for example after failure).
	UPROPERTY()
	bool bShouldLoad = false;

	UPROPERTY()
	TArray<FGP_VillageRuntimeInstance> Instances;
};

USTRUCT(BlueprintType)
struct FGP_VillageGroupRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village")
	FName GroupId = TEXT("Village");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village")
	bool bRequired = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "!bRequired"))
	float SpawnChance = 0.5f;

	// Used when bUsePickCountRange is disabled. Keeping this property preserves
	// existing authored rules and their deterministic selections.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village", meta = (ClampMin = "1", EditCondition = "!bUsePickCountRange"))
	int32 PickCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village")
	bool bUsePickCountRange = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village", meta = (ClampMin = "1", EditCondition = "bUsePickCountRange"))
	int32 MinPickCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village", meta = (ClampMin = "1", EditCondition = "bUsePickCountRange"))
	int32 MaxPickCount = 1;
};

USTRUCT(BlueprintType)
struct FGP_VillageCandidate
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village")
	FName SlotId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village")
	FName GroupId = TEXT("Village");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village", meta = (ClampMin = "0.0"))
	float SelectionWeight = 1.0f;

	// Populated by the layout director from the active preset footprint. Keeping
	// geometry out of the policy makes this usable by future preset types too.
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Village|Footprint")
	TArray<FName> OverlappingSlotIds;
};

USTRUCT(BlueprintType)
struct FGP_VillageSelectionResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Village")
	bool bSucceeded = true;

	UPROPERTY(BlueprintReadOnly, Category = "Village")
	TArray<FName> SelectedSlotIds;

	UPROPERTY(BlueprintReadOnly, Category = "Village")
	TArray<FName> SkippedGroupIds;

	UPROPERTY(BlueprintReadOnly, Category = "Village")
	TArray<FString> Warnings;
};
