// Fill out your copyright notice in the Description page of Project Settings.

#include "Actors/GP_BossSweepTelegraphActor.h"

#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AGP_BossSweepTelegraphActor::AGP_BossSweepTelegraphActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	TelegraphMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("TelegraphMesh"));
	TelegraphMesh->SetupAttachment(SceneRoot);
	TelegraphMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TelegraphMesh->SetCastShadow(false);
	TelegraphMesh->SetReceivesDecals(false);
	TelegraphMesh->SetTranslucentSortPriority(10);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GameTelegraphMaterial(TEXT("/Game/Effects/M_BossTelegraph_Fan.M_BossTelegraph_Fan"));
	if (GameTelegraphMaterial.Succeeded())
	{
		DefaultTelegraphMaterial = GameTelegraphMaterial.Object;
	}
	else
	{
		static ConstructorHelpers::FObjectFinder<UMaterialInterface> FallbackMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		if (FallbackMaterial.Succeeded())
		{
			DefaultTelegraphMaterial = FallbackMaterial.Object;
		}
	}
}

void AGP_BossSweepTelegraphActor::BeginPlay()
{
	Super::BeginPlay();

	SnapToFloor();
}

void AGP_BossSweepTelegraphActor::InitializeSweepTelegraph(float Radius, float ArcAngleDegrees, float LifeSeconds, FLinearColor TelegraphColor, UMaterialInterface* OverrideMaterial)
{
	// The mesh is generated at runtime so this boss warning still works if designer material assets are missing.
	BuildFanMesh(Radius, ArcAngleDegrees, TelegraphColor);

	if (UMaterialInterface* MaterialToUse = ResolveTelegraphMaterial(OverrideMaterial))
	{
		DynamicTelegraphMaterial = UMaterialInstanceDynamic::Create(MaterialToUse, this);
		if (DynamicTelegraphMaterial)
		{
			DynamicTelegraphMaterial->SetVectorParameterValue(TEXT("TelegraphColor"), TelegraphColor);
			DynamicTelegraphMaterial->SetScalarParameterValue(TEXT("Opacity"), TelegraphColor.A);
			TelegraphMesh->SetMaterial(0, DynamicTelegraphMaterial);
		}
		else
		{
			TelegraphMesh->SetMaterial(0, MaterialToUse);
		}
	}

	if (LifeSeconds > 0.0f)
	{
		SetLifeSpan(LifeSeconds + 0.15f);
	}
}

void AGP_BossSweepTelegraphActor::SnapToFloor()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector CurrentLocation = GetActorLocation();
	const FVector TraceStart = CurrentLocation + FVector(0.0f, 0.0f, FloorTraceUpDistance);
	const FVector TraceEnd = CurrentLocation - FVector(0.0f, 0.0f, FloorTraceDownDistance);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(BossSweepTelegraphFloorTrace), false, GetOwner());
	if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		SetActorLocation(HitResult.ImpactPoint + FVector(0.0f, 0.0f, FloorOffset));
		return;
	}

	SetActorLocation(CurrentLocation + FVector(0.0f, 0.0f, FloorOffset));
}

void AGP_BossSweepTelegraphActor::BuildFanMesh(float Radius, float ArcAngleDegrees, FLinearColor TelegraphColor)
{
	const float SafeRadius = FMath::Max(0.0f, Radius);
	const float ClampedArcAngle = FMath::Clamp(ArcAngleDegrees, 1.0f, 360.0f);
	const int32 SegmentCount = FMath::Max(3, ArcSegments);
	const float HalfArcRadians = FMath::DegreesToRadians(ClampedArcAngle * 0.5f);

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	Vertices.Reserve(SegmentCount + 2);
	Triangles.Reserve(SegmentCount * 3);
	Normals.Reserve(SegmentCount + 2);
	UVs.Reserve(SegmentCount + 2);
	VertexColors.Reserve(SegmentCount + 2);
	Tangents.Reserve(SegmentCount + 2);

	Vertices.Add(FVector::ZeroVector);
	Normals.Add(FVector::UpVector);
	UVs.Add(FVector2D(0.5f, 0.0f));
	VertexColors.Add(TelegraphColor);
	Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));

	for (int32 SegmentIndex = 0; SegmentIndex <= SegmentCount; ++SegmentIndex)
	{
		const float Alpha = static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
		const float AngleRadians = FMath::Lerp(-HalfArcRadians, HalfArcRadians, Alpha);
		const FVector VertexLocation(FMath::Cos(AngleRadians) * SafeRadius, FMath::Sin(AngleRadians) * SafeRadius, 0.0f);

		Vertices.Add(VertexLocation);
		Normals.Add(FVector::UpVector);
		UVs.Add(FVector2D(Alpha, 1.0f));
		VertexColors.Add(TelegraphColor);
		Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
	}

	for (int32 SegmentIndex = 1; SegmentIndex <= SegmentCount; ++SegmentIndex)
	{
		Triangles.Add(0);
		Triangles.Add(SegmentIndex);
		Triangles.Add(SegmentIndex + 1);
	}

	TelegraphMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, false);
}

UMaterialInterface* AGP_BossSweepTelegraphActor::ResolveTelegraphMaterial(UMaterialInterface* OverrideMaterial) const
{
	if (OverrideMaterial)
	{
		return OverrideMaterial;
	}

	return DefaultTelegraphMaterial;
}
