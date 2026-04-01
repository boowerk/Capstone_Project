// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "GP_WaterPuddle.generated.h"

UCLASS()
class PROJECT_EDEN_API AGP_WaterPuddle : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AGP_WaterPuddle();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
