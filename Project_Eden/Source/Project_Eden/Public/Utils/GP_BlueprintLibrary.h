// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GP_BlueprintLibrary.generated.h"


UENUM(BlueprintType)
enum class EHitDirection : uint8 {
	Left,
	Right,
	Forward,
	Back
};
/**
 * UGP_BlueprintLibrary
 * 블루프린트 범용 유틸리티 함수
 */
UCLASS()
class PROJECT_EDEN_API UGP_BlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure)
	static EHitDirection GetHitDirection(const FVector& TargetForward, const FVector& ToInstigator); // 피격 방향을 계산

	UFUNCTION(BlueprintPure)
	static FName GetHitDirectionName(const EHitDirection& HitDirection); // EHitDirection to FName으로 변환
};
