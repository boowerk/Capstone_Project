#pragma once

#include "CoreMinimal.h"
#include "GP_MiddleTravelTypes.generated.h"

USTRUCT(BlueprintType)
struct PROJECT_EDEN_API FGPMiddleTravelDestination
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Middle Travel")
	FName ZoneId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Middle Travel")
	int32 ZoneOrder = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Middle Travel")
	FVector_NetQuantize WorldLocation = FVector::ZeroVector;
};
