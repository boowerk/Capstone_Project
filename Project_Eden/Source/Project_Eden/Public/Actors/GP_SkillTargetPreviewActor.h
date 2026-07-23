#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP_SkillTargetPreviewActor.generated.h"

class UDecalComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UNiagaraComponent;
class UNiagaraSystem;
class USceneComponent;

UENUM(BlueprintType)
enum class EGP_SkillTargetPreviewStyle : uint8
{
	Generic,
	Lightning,
	HeavyImpact
};

/**
 * Local-only presentation actor for ground-targeted player skills.
 *
 * The targeting ability owns position and lifetime. This actor owns the
 * production presentation: a ground-conforming decal, a restrained Niagara
 * accent, and valid/invalid feedback.
 */
UCLASS()
class PROJECT_EDEN_API AGP_SkillTargetPreviewActor : public AActor
{
	GENERATED_BODY()

public:
	AGP_SkillTargetPreviewActor();

	virtual void Tick(float DeltaSeconds) override;

	void InitializePreview(
		EGP_SkillTargetPreviewStyle InStyle,
		float InRadius,
		bool bInValidPlacement);
	void UpdatePreview(float InRadius, bool bInValidPlacement);

protected:
	virtual void BeginPlay() override;

private:
	void EnsureDynamicMaterial();
	void RefreshPresentation();
	void ApplyNiagaraColor(const FLinearColor& Color);

	UPROPERTY(VisibleAnywhere, Category = "Skill Preview")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Skill Preview")
	TObjectPtr<UDecalComponent> AreaDecal;

	UPROPERTY(VisibleAnywhere, Category = "Skill Preview")
	TObjectPtr<UNiagaraComponent> AccentEffect;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> PreviewDecalMaterial;

	UPROPERTY()
	TObjectPtr<UNiagaraSystem> LightningPreviewSystem;

	UPROPERTY()
	TObjectPtr<UNiagaraSystem> HeavyImpactPreviewSystem;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PreviewMaterialInstance;

	EGP_SkillTargetPreviewStyle PreviewStyle = EGP_SkillTargetPreviewStyle::Generic;
	float PreviewRadius = 100.0f;
	float ElapsedPreviewTime = 0.0f;
	bool bValidPlacement = true;
	bool bPreviewInitialized = false;
};
