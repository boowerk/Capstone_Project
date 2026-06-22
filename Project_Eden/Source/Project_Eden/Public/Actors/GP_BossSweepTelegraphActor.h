// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP_BossSweepTelegraphActor.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UDecalComponent;
class USceneComponent;

USTRUCT()
struct FGPBossSweepTelegraphSpec
{
	GENERATED_BODY()

	UPROPERTY()
	float Radius = 0.0f;

	UPROPERTY()
	float ArcAngleDegrees = 0.0f;

	UPROPERTY()
	float LifeSeconds = 0.0f;

	UPROPERTY()
	FLinearColor TelegraphColor = FLinearColor::Red;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> OverrideMaterial;

	UPROPERTY()
	bool bInitialized = false;
};

UCLASS()
class PROJECT_EDEN_API AGP_BossSweepTelegraphActor : public AActor
{
	GENERATED_BODY()

public:
	AGP_BossSweepTelegraphActor();

	// Projects the temporary red floor decal used before the Sans boss sweep applies damage.
	UFUNCTION(BlueprintCallable, Category = "Boss|Sweep")
	void InitializeSweepTelegraph(float Radius, float ArcAngleDegrees, float LifeSeconds, FLinearColor TelegraphColor, UMaterialInterface* OverrideMaterial = nullptr);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_TelegraphSpec();

	void ApplyTelegraphSpec();
	void UpdateDecalPresentation();
	void SnapToFloor();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Sweep", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Sweep", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UDecalComponent> WarningDecal;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Sweep|Visual")
	TObjectPtr<UMaterialInterface> DefaultTelegraphMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicTelegraphMaterial;

	UPROPERTY(ReplicatedUsing = OnRep_TelegraphSpec)
	FGPBossSweepTelegraphSpec TelegraphSpec;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Sweep|Placement")
	float FloorTraceUpDistance = 200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Sweep|Placement")
	float FloorTraceDownDistance = 1000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Sweep|Placement")
	float FloorOffset = 4.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Sweep|Visual", meta = (ClampMin = "1.0", Units = "cm"))
	float ProjectionDepth = 180.0f;
};
