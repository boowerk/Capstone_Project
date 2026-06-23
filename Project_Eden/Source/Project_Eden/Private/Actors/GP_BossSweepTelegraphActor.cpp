// Fill out your copyright notice in the Description page of Project Settings.

#include "Actors/GP_BossSweepTelegraphActor.h"

#include "Components/DecalComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AGP_BossSweepTelegraphActor::AGP_BossSweepTelegraphActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetNetUpdateFrequency(30.0f);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	WarningDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("WarningDecal"));
	WarningDecal->SetupAttachment(SceneRoot);
	// The material's UV-forward axis is local +Y, so -90 yaw aligns its fan with the owning boss's +X forward.
	WarningDecal->SetRelativeRotation(FRotator(-90.0f, -90.0f, 0.0f));
	WarningDecal->SetFadeScreenSize(0.001f);
	WarningDecal->SetSortOrder(8);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DecalMaterialFinder(
		TEXT("/Game/Effects/M_BossSweepTelegraph_Decal.M_BossSweepTelegraph_Decal"));
	if (DecalMaterialFinder.Succeeded())
	{
		DefaultTelegraphMaterial = DecalMaterialFinder.Object;
		WarningDecal->SetDecalMaterial(DefaultTelegraphMaterial);
	}
}

void AGP_BossSweepTelegraphActor::BeginPlay()
{
	Super::BeginPlay();

	SnapToFloor();
	WarningDecal->SetVisibility(false);
	if (TelegraphSpec.bInitialized)
	{
		ApplyTelegraphSpec();
	}
}

void AGP_BossSweepTelegraphActor::InitializeSweepTelegraph(float Radius, float ArcAngleDegrees, float LifeSeconds, FLinearColor TelegraphColor, UMaterialInterface* OverrideMaterial)
{
	TelegraphSpec.Radius = Radius;
	TelegraphSpec.ArcAngleDegrees = ArcAngleDegrees;
	TelegraphSpec.LifeSeconds = LifeSeconds;
	TelegraphSpec.TelegraphColor = TelegraphColor;
	TelegraphSpec.OverrideMaterial = OverrideMaterial;
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

	UpdateDecalPresentation();
}

void AGP_BossSweepTelegraphActor::UpdateDecalPresentation()
{
	SnapToFloor();

	const float SafeRadius = FMath::Max(1.0f, TelegraphSpec.Radius);
	const float HalfAngleDegrees = FMath::Clamp(TelegraphSpec.ArcAngleDegrees * 0.5f, 0.0f, 180.0f);
	WarningDecal->DecalSize = FVector(FMath::Max(1.0f, ProjectionDepth), SafeRadius, SafeRadius);
	WarningDecal->DecalColor = TelegraphSpec.TelegraphColor;

	UMaterialInterface* MaterialToUse = TelegraphSpec.OverrideMaterial
		? TelegraphSpec.OverrideMaterial.Get()
		: DefaultTelegraphMaterial.Get();
	if (!IsValid(MaterialToUse))
	{
		WarningDecal->SetVisibility(false);
		return;
	}

	// Every client creates its own MID because replicated material instances are not stable presentation state.
	DynamicTelegraphMaterial = UMaterialInstanceDynamic::Create(MaterialToUse, this);
	UMaterialInterface* ResolvedMaterial = DynamicTelegraphMaterial
		? static_cast<UMaterialInterface*>(DynamicTelegraphMaterial.Get())
		: MaterialToUse;
	if (DynamicTelegraphMaterial)
	{
		DynamicTelegraphMaterial->SetVectorParameterValue(
			TEXT("TelegraphColor"),
			FLinearColor(TelegraphSpec.TelegraphColor.R, TelegraphSpec.TelegraphColor.G, TelegraphSpec.TelegraphColor.B, 1.0f));
		DynamicTelegraphMaterial->SetScalarParameterValue(TEXT("TelegraphOpacity"), TelegraphSpec.TelegraphColor.A);
		DynamicTelegraphMaterial->SetScalarParameterValue(TEXT("HalfAngleCos"), FMath::Cos(FMath::DegreesToRadians(HalfAngleDegrees)));
	}

	WarningDecal->SetDecalMaterial(ResolvedMaterial);
	WarningDecal->SetVisibility(true);
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
	FCollisionObjectQueryParams FloorObjectQuery;
	// Match Ground Hands by projecting only onto static level geometry, never characters or transient effects.
	FloorObjectQuery.AddObjectTypesToQuery(ECC_WorldStatic);
	if (World->LineTraceSingleByObjectType(HitResult, TraceStart, TraceEnd, FloorObjectQuery, QueryParams))
	{
		SetActorLocation(HitResult.ImpactPoint + FVector(0.0f, 0.0f, FloorOffset));
		return;
	}

	SetActorLocation(CurrentLocation + FVector(0.0f, 0.0f, FloorOffset));
}
