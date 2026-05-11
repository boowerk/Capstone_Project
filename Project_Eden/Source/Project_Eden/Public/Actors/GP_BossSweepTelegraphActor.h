// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP_BossSweepTelegraphActor.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UProceduralMeshComponent;
class USceneComponent;

UCLASS()
class PROJECT_EDEN_API AGP_BossSweepTelegraphActor : public AActor
{
	GENERATED_BODY()

public:
	AGP_BossSweepTelegraphActor();

	// Builds the warning fan used before the Sans boss sweep attack applies damage.
	UFUNCTION(BlueprintCallable, Category = "Boss|Sweep")
	void InitializeSweepTelegraph(float Radius, float ArcAngleDegrees, float LifeSeconds, FLinearColor TelegraphColor, UMaterialInterface* OverrideMaterial = nullptr);

protected:
	virtual void BeginPlay() override;

private:
	void SnapToFloor();
	void BuildFanMesh(float Radius, float ArcAngleDegrees, FLinearColor TelegraphColor);
	UMaterialInterface* ResolveTelegraphMaterial(UMaterialInterface* OverrideMaterial) const;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Sweep", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Sweep", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UProceduralMeshComponent> TelegraphMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Sweep")
	TObjectPtr<UMaterialInterface> DefaultTelegraphMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicTelegraphMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Sweep")
	float FloorTraceUpDistance = 200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Sweep")
	float FloorTraceDownDistance = 1000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Sweep")
	float FloorOffset = 4.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Sweep")
	int32 ArcSegments = 36;
};
