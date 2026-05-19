// Fill out your copyright notice in the Description page of Project Settings.

#include "Actors/GP_BossSweepTelegraphActor.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

AGP_BossSweepTelegraphActor::AGP_BossSweepTelegraphActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetNetUpdateFrequency(30.0f);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void AGP_BossSweepTelegraphActor::BeginPlay()
{
	Super::BeginPlay();

	SnapToFloor();
}

void AGP_BossSweepTelegraphActor::InitializeSweepTelegraph(float Radius, float ArcAngleDegrees, float LifeSeconds, FLinearColor TelegraphColor, UMaterialInterface* OverrideMaterial)
{
	// OverrideMaterial is intentionally ignored because this warning now uses debug range lines instead of mesh materials.
	(void)OverrideMaterial;
	TelegraphSpec.Radius = Radius;
	TelegraphSpec.ArcAngleDegrees = ArcAngleDegrees;
	TelegraphSpec.LifeSeconds = LifeSeconds;
	TelegraphSpec.TelegraphColor = TelegraphColor;
	TelegraphSpec.bInitialized = true;

	ApplyTelegraphSpec();

	if (HasAuthority())
	{
		ForceNetUpdate();
		if (LifeSeconds > 0.0f)
		{
			SetLifeSpan(LifeSeconds + 0.15f);
		}
	}
}

void AGP_BossSweepTelegraphActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGP_BossSweepTelegraphActor, TelegraphSpec);
}

void AGP_BossSweepTelegraphActor::OnRep_TelegraphSpec()
{
	ApplyTelegraphSpec();
}

void AGP_BossSweepTelegraphActor::ApplyTelegraphSpec()
{
	if (!TelegraphSpec.bInitialized)
	{
		return;
	}

	// Show the attack range with debug lines so the warning stays visible even when mesh materials fail to render.
	DrawAttackRangePreview();
}

void AGP_BossSweepTelegraphActor::DrawAttackRangePreview()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	SnapToFloor();

	const float SafeRadius = FMath::Max(0.0f, TelegraphSpec.Radius);
	const float HalfAngleDegrees = FMath::Clamp(TelegraphSpec.ArcAngleDegrees * 0.5f, 0.0f, 180.0f);
	const int32 SegmentCount = FMath::Max(8, ArcSegments);
	const float LifeSeconds = FMath::Max(TelegraphSpec.LifeSeconds, 0.05f);
	const FColor RangeColor = FColor::Red;
	const float LineThickness = 8.0f;
	const FVector Origin = GetActorLocation() + FVector(0.0f, 0.0f, 18.0f);
	FVector ForwardVector = GetActorForwardVector().GetSafeNormal2D();
	if (ForwardVector.IsNearlyZero())
	{
		ForwardVector = FVector::ForwardVector;
	}

	DrawDebugSphere(World, Origin, 24.0f, 12, RangeColor, false, LifeSeconds, 0, LineThickness);

	FVector PreviousArcPoint = FVector::ZeroVector;
	for (int32 SegmentIndex = 0; SegmentIndex <= SegmentCount; ++SegmentIndex)
	{
		const float Alpha = static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
		const float AngleDegrees = FMath::Lerp(-HalfAngleDegrees, HalfAngleDegrees, Alpha);
		const FVector Direction = FRotator(0.0f, AngleDegrees, 0.0f).RotateVector(ForwardVector).GetSafeNormal();
		const FVector ArcPoint = Origin + Direction * SafeRadius;

		if (SegmentIndex > 0)
		{
			DrawDebugLine(World, PreviousArcPoint, ArcPoint, RangeColor, false, LifeSeconds, 0, LineThickness);
		}

		const bool bBoundaryLine = SegmentIndex == 0 || SegmentIndex == SegmentCount;
		const bool bGuideLine = SegmentIndex % 6 == 0;
		if (bBoundaryLine || bGuideLine)
		{
			DrawDebugLine(World, Origin, ArcPoint, RangeColor, false, LifeSeconds, 0, LineThickness);
		}

		PreviousArcPoint = ArcPoint;
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
