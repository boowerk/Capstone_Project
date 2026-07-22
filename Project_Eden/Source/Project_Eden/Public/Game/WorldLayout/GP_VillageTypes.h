#pragma once

#include "CoreMinimal.h"
#include "GP_VillageTypes.generated.h"

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
