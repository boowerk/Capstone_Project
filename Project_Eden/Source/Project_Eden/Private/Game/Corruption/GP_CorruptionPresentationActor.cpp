#include "Game/Corruption/GP_CorruptionPresentationActor.h"

#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Game/Corruption/GP_WorldCorruptionComponent.h"
#include "Game/GP_GameState.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialParameters.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"

AGP_CorruptionPresentationActor::AGP_CorruptionPresentationActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(false);
}

void AGP_CorruptionPresentationActor::BeginPlay()
{
	Super::BeginPlay();
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	TryResolveAndBind();
	if (UWorld* World = GetWorld())
	{
		// Streaming levels and replicated GameState may arrive after this actor, so retry until all targets exist.
		World->GetTimerManager().SetTimer(
			ResolveTargetsTimerHandle,
			this,
			&ThisClass::TryResolveAndBind,
			0.5f,
			true);
	}
}

void AGP_CorruptionPresentationActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ResolveTargetsTimerHandle);
	}
	UnbindFromWorldCorruption();
	RestoreBaselinePresentation();
	Super::EndPlay(EndPlayReason);
}

void AGP_CorruptionPresentationActor::RefreshPresentationTargets()
{
	ResolveEnvironmentActors();
	BindToWorldCorruption();
	if (BoundWorldCorruption.IsValid())
	{
		ApplyCorruptionPresentation(BoundWorldCorruption->GetWorldCorruptionNormalized());
	}
}

void AGP_CorruptionPresentationActor::HandleWorldCorruptionChanged(float Corruption, float NormalizedCorruption)
{
	ApplyCorruptionPresentation(NormalizedCorruption);
}

void AGP_CorruptionPresentationActor::TryResolveAndBind()
{
	BindToWorldCorruption();
	ResolveEnvironmentActors();

	if (BoundWorldCorruption.IsValid())
	{
		ApplyCorruptionPresentation(BoundWorldCorruption->GetWorldCorruptionNormalized());
	}

	const bool bHasSkyTarget = !bAffectSkyAtmosphere || IsValid(SkyAtmosphereActor);
	const bool bHasFogTarget = !bAffectHeightFog || IsValid(HeightFogActor);
	const bool bHasSkyboxTarget = !bAffectSkyboxMaterials || !SkyboxMaterialBindings.IsEmpty();
	if (BoundWorldCorruption.IsValid() && bHasSkyTarget && bHasFogTarget && bHasSkyboxTarget)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(ResolveTargetsTimerHandle);
		}
	}
}

void AGP_CorruptionPresentationActor::BindToWorldCorruption()
{
	UWorld* World = GetWorld();
	AGP_GameState* GameState = World ? World->GetGameState<AGP_GameState>() : nullptr;
	UGP_WorldCorruptionComponent* Corruption = IsValid(GameState) ? GameState->GetWorldCorruptionComponent() : nullptr;
	if (!IsValid(Corruption) || BoundWorldCorruption.Get() == Corruption)
	{
		return;
	}

	UnbindFromWorldCorruption();
	BoundWorldCorruption = Corruption;
	Corruption->OnWorldCorruptionChanged.AddUniqueDynamic(this, &ThisClass::HandleWorldCorruptionChanged);
}

void AGP_CorruptionPresentationActor::UnbindFromWorldCorruption()
{
	if (UGP_WorldCorruptionComponent* Corruption = BoundWorldCorruption.Get())
	{
		Corruption->OnWorldCorruptionChanged.RemoveDynamic(this, &ThisClass::HandleWorldCorruptionChanged);
	}
	BoundWorldCorruption.Reset();
}

void AGP_CorruptionPresentationActor::ResolveEnvironmentActors()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (bAffectSkyAtmosphere && !IsValid(SkyAtmosphereActor))
	{
		for (TActorIterator<ASkyAtmosphere> It(World); It; ++It)
		{
			SkyAtmosphereActor = *It;
			break;
		}
	}

	if (bAffectHeightFog && !IsValid(HeightFogActor))
	{
		for (TActorIterator<AExponentialHeightFog> It(World); It; ++It)
		{
			HeightFogActor = *It;
			break;
		}
	}

	if (IsValid(SkyAtmosphereActor))
	{
		CaptureSkyBaseline(SkyAtmosphereActor->GetComponent());
	}
	if (IsValid(HeightFogActor))
	{
		CaptureFogBaseline(HeightFogActor->GetComponent());
	}
	ResolveSkyboxMaterials();
}

void AGP_CorruptionPresentationActor::ResolveSkyboxMaterials()
{
	UWorld* World = GetWorld();
	if (!bAffectSkyboxMaterials || !World || !SkyboxMaterialBindings.IsEmpty())
	{
		return;
	}

	for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
	{
		UStaticMeshComponent* MeshComponent = It->GetStaticMeshComponent();
		if (!IsValid(MeshComponent)
			|| !IsValid(MeshComponent->GetStaticMesh())
			|| !MeshComponent->GetStaticMesh()->GetName().Contains(TEXT("Skybox"), ESearchCase::IgnoreCase))
		{
			continue;
		}

		for (int32 MaterialIndex = 0; MaterialIndex < MeshComponent->GetNumMaterials(); ++MaterialIndex)
		{
			UMaterialInterface* OriginalMaterial = MeshComponent->GetMaterial(MaterialIndex);
			if (!IsValid(OriginalMaterial))
			{
				continue;
			}

			FGPCorruptionSkyboxMaterialBinding Binding;
			Binding.MeshComponent = MeshComponent;
			Binding.OriginalMaterial = OriginalMaterial;
			Binding.MaterialIndex = MaterialIndex;
			Binding.bHasTintParameter = OriginalMaterial->GetVectorParameterValue(
				FHashedMaterialParameterInfo(SkyboxTintParameterName),
				Binding.BaseTint);
			Binding.bHasBrightnessParameter = OriginalMaterial->GetScalarParameterValue(
				FHashedMaterialParameterInfo(SkyboxBrightnessParameterName),
				Binding.BaseBrightness);
			if (!Binding.bHasTintParameter && !Binding.bHasBrightnessParameter)
			{
				continue;
			}

			Binding.DynamicMaterial = UMaterialInstanceDynamic::Create(OriginalMaterial, this);
			if (!IsValid(Binding.DynamicMaterial))
			{
				continue;
			}

			MeshComponent->SetMaterial(MaterialIndex, Binding.DynamicMaterial);
			SkyboxMaterialBindings.Add(MoveTemp(Binding));
		}
	}

	if (!SkyboxMaterialBindings.IsEmpty())
	{
		// Newly streamed skybox materials must receive the current value even when corruption itself did not change.
		AppliedCorruptionNormalized = -1.0f;
	}
}

void AGP_CorruptionPresentationActor::CaptureSkyBaseline(USkyAtmosphereComponent* SkyComponent)
{
	if (!IsValid(SkyComponent) || bSkyBaselineCaptured)
	{
		return;
	}

	BaseSkyLuminanceFactor = SkyComponent->SkyLuminanceFactor;
	BaseSkyAerialLuminanceFactor = SkyComponent->SkyAndAerialPerspectiveLuminanceFactor;
	BaseMieScatteringScale = SkyComponent->MieScatteringScale;
	BaseRayleighScatteringScale = SkyComponent->RayleighScatteringScale;
	BaseHeightFogContribution = SkyComponent->HeightFogContribution;
	bSkyBaselineCaptured = true;
	// Force the current corruption value onto a target that streamed in after the last apply.
	AppliedCorruptionNormalized = -1.0f;
}

void AGP_CorruptionPresentationActor::CaptureFogBaseline(UExponentialHeightFogComponent* FogComponent)
{
	if (!IsValid(FogComponent) || bFogBaselineCaptured)
	{
		return;
	}

	BaseFogDensity = FogComponent->FogDensity;
	BaseFogInscatteringColor = FogComponent->FogInscatteringLuminance;
	BaseDirectionalInscatteringColor = FogComponent->DirectionalInscatteringLuminance;
	bFogBaselineCaptured = true;
	AppliedCorruptionNormalized = -1.0f;
}

void AGP_CorruptionPresentationActor::ApplyCorruptionPresentation(float NormalizedCorruption)
{
	const float Alpha = FMath::Clamp(NormalizedCorruption, 0.0f, 1.0f);
	if (FMath::IsNearlyEqual(AppliedCorruptionNormalized, Alpha, 0.001f))
	{
		return;
	}
	AppliedCorruptionNormalized = Alpha;

	if (bAffectSkyAtmosphere && IsValid(SkyAtmosphereActor) && bSkyBaselineCaptured)
	{
		if (USkyAtmosphereComponent* Sky = SkyAtmosphereActor->GetComponent())
		{
			const FLinearColor ColorMultiplier = FLinearColor::LerpUsingHSV(FLinearColor::White, CorruptedSkyColorMultiplier, Alpha);
			Sky->SetSkyLuminanceFactor(BaseSkyLuminanceFactor * ColorMultiplier);
			Sky->SetSkyAndAerialPerspectiveLuminanceFactor(BaseSkyAerialLuminanceFactor * ColorMultiplier);
			Sky->SetMieScatteringScale(BaseMieScatteringScale * FMath::Lerp(1.0f, CorruptedMieScatteringMultiplier, Alpha));
			Sky->SetRayleighScatteringScale(BaseRayleighScatteringScale * FMath::Lerp(1.0f, CorruptedRayleighScatteringMultiplier, Alpha));
			Sky->SetHeightFogContribution(FMath::Lerp(BaseHeightFogContribution, 1.0f, Alpha));
		}
	}

	if (bAffectHeightFog && IsValid(HeightFogActor) && bFogBaselineCaptured)
	{
		if (UExponentialHeightFogComponent* Fog = HeightFogActor->GetComponent())
		{
			Fog->SetFogDensity(BaseFogDensity * FMath::Lerp(1.0f, CorruptedFogDensityMultiplier, Alpha));
			Fog->SetFogInscatteringColor(FLinearColor::LerpUsingHSV(BaseFogInscatteringColor, CorruptedFogColor, Alpha));
			Fog->SetDirectionalInscatteringColor(FLinearColor::LerpUsingHSV(BaseDirectionalInscatteringColor, CorruptedFogColor, Alpha));
		}
	}

	if (bAffectSkyboxMaterials)
	{
		const FLinearColor SkyboxColorMultiplier = FLinearColor::LerpUsingHSV(
			FLinearColor::White,
			CorruptedSkyboxTintMultiplier,
			Alpha);
		for (FGPCorruptionSkyboxMaterialBinding& Binding : SkyboxMaterialBindings)
		{
			if (!IsValid(Binding.DynamicMaterial))
			{
				continue;
			}
			if (Binding.bHasTintParameter)
			{
				Binding.DynamicMaterial->SetVectorParameterValue(
					SkyboxTintParameterName,
					Binding.BaseTint * SkyboxColorMultiplier);
			}
			if (Binding.bHasBrightnessParameter)
			{
				Binding.DynamicMaterial->SetScalarParameterValue(
					SkyboxBrightnessParameterName,
					Binding.BaseBrightness * FMath::Lerp(1.0f, CorruptedSkyboxBrightnessMultiplier, Alpha));
			}
		}
	}

	BP_OnCorruptionPresentationChanged(Alpha);
}

void AGP_CorruptionPresentationActor::RestoreBaselinePresentation()
{
	if (IsValid(SkyAtmosphereActor) && bSkyBaselineCaptured)
	{
		if (USkyAtmosphereComponent* Sky = SkyAtmosphereActor->GetComponent())
		{
			Sky->SetSkyLuminanceFactor(BaseSkyLuminanceFactor);
			Sky->SetSkyAndAerialPerspectiveLuminanceFactor(BaseSkyAerialLuminanceFactor);
			Sky->SetMieScatteringScale(BaseMieScatteringScale);
			Sky->SetRayleighScatteringScale(BaseRayleighScatteringScale);
			Sky->SetHeightFogContribution(BaseHeightFogContribution);
		}
	}

	if (IsValid(HeightFogActor) && bFogBaselineCaptured)
	{
		if (UExponentialHeightFogComponent* Fog = HeightFogActor->GetComponent())
		{
			Fog->SetFogDensity(BaseFogDensity);
			Fog->SetFogInscatteringColor(BaseFogInscatteringColor);
			Fog->SetDirectionalInscatteringColor(BaseDirectionalInscatteringColor);
		}
	}

	for (const FGPCorruptionSkyboxMaterialBinding& Binding : SkyboxMaterialBindings)
	{
		if (IsValid(Binding.MeshComponent)
			&& Binding.MaterialIndex != INDEX_NONE
			&& Binding.MeshComponent->GetMaterial(Binding.MaterialIndex) == Binding.DynamicMaterial)
		{
			Binding.MeshComponent->SetMaterial(Binding.MaterialIndex, Binding.OriginalMaterial);
		}
	}
	SkyboxMaterialBindings.Reset();
}
