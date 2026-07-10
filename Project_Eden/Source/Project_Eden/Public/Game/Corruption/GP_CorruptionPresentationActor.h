#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP_CorruptionPresentationActor.generated.h"

class AExponentialHeightFog;
class ASkyAtmosphere;
class UExponentialHeightFogComponent;
class UGP_WorldCorruptionComponent;
class USkyAtmosphereComponent;

/** Client-side visual adapter for the replicated corruption state. Safe to auto-spawn from GP_GameMode. */
UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_CorruptionPresentationActor : public AActor
{
	GENERATED_BODY()

public:
	AGP_CorruptionPresentationActor();

	UFUNCTION(BlueprintPure, Category = "World Corruption|Presentation")
	float GetAppliedCorruptionNormalized() const { return AppliedCorruptionNormalized; }

	UFUNCTION(BlueprintCallable, Category = "World Corruption|Presentation")
	void RefreshPresentationTargets();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Corruption|Presentation")
	bool bAffectSkyAtmosphere = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Corruption|Presentation")
	bool bAffectHeightFog = true;

	// Multiplies the clean sky luminance at maximum corruption.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Corruption|Presentation", meta = (EditCondition = "bAffectSkyAtmosphere"))
	FLinearColor CorruptedSkyColorMultiplier = FLinearColor(0.55f, 0.16f, 0.22f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Corruption|Presentation", meta = (EditCondition = "bAffectSkyAtmosphere", ClampMin = "0.0"))
	float CorruptedMieScatteringMultiplier = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Corruption|Presentation", meta = (EditCondition = "bAffectSkyAtmosphere", ClampMin = "0.0"))
	float CorruptedRayleighScatteringMultiplier = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Corruption|Presentation", meta = (EditCondition = "bAffectHeightFog"))
	FLinearColor CorruptedFogColor = FLinearColor(0.16f, 0.025f, 0.04f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Corruption|Presentation", meta = (EditCondition = "bAffectHeightFog", ClampMin = "0.0"))
	float CorruptedFogDensityMultiplier = 2.5f;

	// Allows a BP child to drive custom skybox material parameters without coupling them to the domain component.
	UFUNCTION(BlueprintImplementableEvent, Category = "World Corruption|Presentation")
	void BP_OnCorruptionPresentationChanged(float NormalizedCorruption);

private:
	UPROPERTY(EditInstanceOnly, Category = "World Corruption|Presentation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ASkyAtmosphere> SkyAtmosphereActor;

	UPROPERTY(EditInstanceOnly, Category = "World Corruption|Presentation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AExponentialHeightFog> HeightFogActor;

	TWeakObjectPtr<UGP_WorldCorruptionComponent> BoundWorldCorruption;
	FTimerHandle ResolveTargetsTimerHandle;
	float AppliedCorruptionNormalized = -1.0f;
	bool bSkyBaselineCaptured = false;
	bool bFogBaselineCaptured = false;

	FLinearColor BaseSkyLuminanceFactor = FLinearColor::White;
	FLinearColor BaseSkyAerialLuminanceFactor = FLinearColor::White;
	float BaseMieScatteringScale = 1.0f;
	float BaseRayleighScatteringScale = 1.0f;
	float BaseHeightFogContribution = 1.0f;

	float BaseFogDensity = 0.0f;
	FLinearColor BaseFogInscatteringColor = FLinearColor::White;
	FLinearColor BaseDirectionalInscatteringColor = FLinearColor::White;

	UFUNCTION()
	void HandleWorldCorruptionChanged(float Corruption, float NormalizedCorruption);

	void TryResolveAndBind();
	void BindToWorldCorruption();
	void UnbindFromWorldCorruption();
	void ResolveEnvironmentActors();
	void CaptureSkyBaseline(USkyAtmosphereComponent* SkyComponent);
	void CaptureFogBaseline(UExponentialHeightFogComponent* FogComponent);
	void ApplyCorruptionPresentation(float NormalizedCorruption);
	void RestoreBaselinePresentation();
};
