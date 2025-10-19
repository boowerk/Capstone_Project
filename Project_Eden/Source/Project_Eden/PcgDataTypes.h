#pragma once

#include "CoreMinimal.h"
#include "PcgDataTypes.generated.h"

USTRUCT(BlueprintType)
struct PROJECT_EDEN_API FPCGScatterParams
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) float ScaleMin = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float ScaleMax = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float RotationMin = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float RotationMax = 0.0f;
};
