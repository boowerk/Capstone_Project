#include "Actors/GP_SkillTargetPreviewActor.h"

#include "Components/DecalComponent.h"
#include "Components/SceneComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FName TelegraphColorParameter(TEXT("TelegraphColor"));
	const FName TelegraphOpacityParameter(TEXT("TelegraphOpacity"));
	const FName NormalizedRadiusParameter(TEXT("NormalizedRadius"));
	const FName HalfAngleCosParameter(TEXT("HalfAngleCos"));
	const FName NiagaraScaleParameter(TEXT("User.Scale_All"));
	constexpr float AccentScaleFraction = 0.60f;

	const FLinearColor LightningColor(0.08f, 0.58f, 1.0f, 1.0f);
	const FLinearColor HeavyImpactColor(1.0f, 0.42f, 0.055f, 1.0f);
	const FLinearColor GenericColor(0.18f, 0.78f, 0.72f, 1.0f);
	const FLinearColor InvalidColor(0.88f, 0.025f, 0.018f, 1.0f);
}

AGP_SkillTargetPreviewActor::AGP_SkillTargetPreviewActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	bReplicates = false;
	SetActorEnableCollision(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	AreaDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("AreaDecal"));
	AreaDecal->SetupAttachment(SceneRoot);
	AreaDecal->SetRelativeLocation(FVector(0.0f, 0.0f, 8.0f));
	AreaDecal->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	AreaDecal->DecalSize = FVector(96.0f, PreviewRadius, PreviewRadius);
	AreaDecal->SetSortOrder(20);
	AreaDecal->SetFadeScreenSize(0.001f);
	AreaDecal->SetVisibility(true);

	AccentEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("AccentEffect"));
	AccentEffect->SetupAttachment(SceneRoot);
	AccentEffect->SetRelativeLocation(FVector(0.0f, 0.0f, 4.0f));
	AccentEffect->SetAutoActivate(false);
	AccentEffect->SetVisibility(false);
	AccentEffect->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AccentEffect->SetGenerateOverlapEvents(false);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DecalMaterialFinder(
		TEXT("/Game/Effects/M_BossSweepTelegraph_Decal.M_BossSweepTelegraph_Decal"));
	if (DecalMaterialFinder.Succeeded())
	{
		PreviewDecalMaterial = DecalMaterialFinder.Object;
		AreaDecal->SetDecalMaterial(PreviewDecalMaterial);
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> LightningSystemFinder(
		TEXT("/Game/Imported_VFX/Free_Magic/VFX_Niagara/NS_Free_Magic_Circle2.NS_Free_Magic_Circle2"));
	if (LightningSystemFinder.Succeeded())
	{
		LightningPreviewSystem = LightningSystemFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> HeavyImpactSystemFinder(
		TEXT("/Game/Imported_VFX/Free_Magic/VFX_Niagara/NS_Free_Magic_Circle1.NS_Free_Magic_Circle1"));
	if (HeavyImpactSystemFinder.Succeeded())
	{
		HeavyImpactPreviewSystem = HeavyImpactSystemFinder.Object;
	}
}

void AGP_SkillTargetPreviewActor::BeginPlay()
{
	Super::BeginPlay();

	EnsureDynamicMaterial();
}

void AGP_SkillTargetPreviewActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bPreviewInitialized)
	{
		return;
	}

	ElapsedPreviewTime += DeltaSeconds;
	EnsureDynamicMaterial();
	if (!PreviewMaterialInstance)
	{
		return;
	}

	const float PulseSpeed = bValidPlacement ? 2.6f : 7.0f;
	const float PulseAmount = bValidPlacement ? 0.035f : 0.07f;
	const float BaseOpacity = bValidPlacement ? 0.28f : 0.20f;
	const float PulsedOpacity =
		BaseOpacity + FMath::Sin(ElapsedPreviewTime * PulseSpeed) * PulseAmount;
	PreviewMaterialInstance->SetScalarParameterValue(
		TelegraphOpacityParameter,
		PulsedOpacity);
}

void AGP_SkillTargetPreviewActor::InitializePreview(
	EGP_SkillTargetPreviewStyle InStyle,
	float InRadius,
	bool bInValidPlacement)
{
	PreviewStyle = InStyle;
	PreviewRadius = FMath::Max(InRadius, 1.0f);
	bValidPlacement = bInValidPlacement;
	bPreviewInitialized = true;

	if (AreaDecal)
	{
		AreaDecal->DecalSize = FVector(96.0f, PreviewRadius, PreviewRadius);
		AreaDecal->SetVisibility(true);
		AreaDecal->MarkRenderStateDirty();
	}

	RefreshPresentation();
}

void AGP_SkillTargetPreviewActor::UpdatePreview(
	float InRadius,
	bool bInValidPlacement)
{
	const float SafeRadius = FMath::Max(InRadius, 1.0f);
	const bool bRadiusChanged = !FMath::IsNearlyEqual(PreviewRadius, SafeRadius, 0.5f);
	const bool bValidityChanged = bValidPlacement != bInValidPlacement;

	PreviewRadius = SafeRadius;
	bValidPlacement = bInValidPlacement;

	if (AreaDecal)
	{
		AreaDecal->DecalSize = FVector(96.0f, PreviewRadius, PreviewRadius);
		AreaDecal->SetVisibility(true);
		AreaDecal->MarkRenderStateDirty();
	}

	if (bRadiusChanged || bValidityChanged)
	{
		RefreshPresentation();
	}
}

void AGP_SkillTargetPreviewActor::EnsureDynamicMaterial()
{
	if (PreviewMaterialInstance || !AreaDecal || !PreviewDecalMaterial)
	{
		return;
	}

	PreviewMaterialInstance = AreaDecal->CreateDynamicMaterialInstance();
}

void AGP_SkillTargetPreviewActor::RefreshPresentation()
{
	if (!bPreviewInitialized)
	{
		return;
	}

	EnsureDynamicMaterial();

	const FLinearColor StyleColor =
		!bValidPlacement
			? InvalidColor
			: PreviewStyle == EGP_SkillTargetPreviewStyle::Lightning
				? LightningColor
				: PreviewStyle == EGP_SkillTargetPreviewStyle::HeavyImpact
					? HeavyImpactColor
					: GenericColor;

	if (PreviewMaterialInstance)
	{
		PreviewMaterialInstance->SetVectorParameterValue(
			TelegraphColorParameter,
			StyleColor);
		PreviewMaterialInstance->SetScalarParameterValue(
			TelegraphOpacityParameter,
			bValidPlacement ? 0.28f : 0.20f);
		PreviewMaterialInstance->SetScalarParameterValue(
			NormalizedRadiusParameter,
			0.5f);
		PreviewMaterialInstance->SetScalarParameterValue(
			HalfAngleCosParameter,
			-1.0f);
	}

	if (AccentEffect)
	{
		AccentEffect->DeactivateImmediate();
		AccentEffect->SetVisibility(false);
	}
}

void AGP_SkillTargetPreviewActor::ApplyNiagaraColor(
	const FLinearColor& Color)
{
	if (!AccentEffect)
	{
		return;
	}

	static const FName ColorParameters[] = {
		TEXT("User.Color_Circle"),
		TEXT("User.Color_Smoke"),
		TEXT("User.Color_Sparks1"),
		TEXT("User.Color_Spiral1"),
		TEXT("User.Color_Mesh1"),
		TEXT("User.Color_Ray"),
		TEXT("User.Color_Sparks2")
	};

	for (const FName ParameterName : ColorParameters)
	{
		AccentEffect->SetVariableLinearColor(ParameterName, Color);
	}
}
