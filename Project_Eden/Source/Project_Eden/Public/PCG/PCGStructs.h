#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PCGStructs.generated.h"

USTRUCT(BlueprintType)
struct FPCGItemDetails
{
    GENERATED_BODY()

    // 스폰될 스태틱 메시
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG Item")
    TSoftObjectPtr<UStaticMesh> Mesh;

    // 스폰 가중치
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG Item", meta = (ClampMin = "0.0"))
    float Weight = 1.0f;

    // 최소 스케일
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG Item")
    FVector MinScale = FVector(1.0f);

    // 최대 스케일
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG Item")
    FVector MaxScale = FVector(1.0f);

    // 최소 회전
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG Item")
    FRotator MinRotation = FRotator::ZeroRotator;

    // 최대 회전
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG Item")
    FRotator MaxRotation = FRotator::ZeroRotator;

    // 오프셋
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG Item")
    FVector MinOffset = FVector(0.0f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG Item")
    FVector MaxOffset = FVector(0.0f);

};