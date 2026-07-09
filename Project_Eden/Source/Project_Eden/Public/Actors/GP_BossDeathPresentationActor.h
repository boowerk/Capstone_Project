#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "GP_BossDeathPresentationActor.generated.h"

class UAnimationAsset;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UNiagaraSystem;
class USceneComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EGPBossDeathPresentationStyle : uint8
{
	None UMETA(DisplayName = "None"),
	Auto UMETA(DisplayName = "Auto"),
	CrystalSeraph UMETA(DisplayName = "Crystal Seraph"),
	Sans UMETA(DisplayName = "Sans"),
	DarkArmorKnight UMETA(DisplayName = "Dark Armor Knight"),
	Matador UMETA(DisplayName = "Matador")
};

USTRUCT(BlueprintType)
struct FGPBossDeathPresentationSpawnSettings
{
	GENERATED_BODY()

	// Presentation actors are client-local, so the boss can despawn while the clear effect continues.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Death Presentation", meta = (ClampMin = "0.1", Units = "s"))
	float LifeSpanSeconds = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Death Presentation", meta = (ClampMin = "0.01", UIMin = "0.25", UIMax = "3.0"))
	float UniformScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Death Presentation")
	bool bHideSourceMesh = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Death Presentation", meta = (ClampMin = "0.0", Units = "s"))
	float SourceMeshHideDelaySeconds = 0.08f;

	// Optional per-BP override lets designers reuse the same presentation code with a stronger Niagara burst.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Death Presentation")
	TObjectPtr<UNiagaraSystem> OverrideBurstNiagara;
};

/** Lightweight local-only actor that turns a boss death into readable clear feedback without requiring death animations. */
UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_BossDeathPresentationActor : public AActor
{
	GENERATED_BODY()

public:
	AGP_BossDeathPresentationActor();

	virtual void Tick(float DeltaSeconds) override;

	void InitializePresentation(
		EGPBossDeathPresentationStyle InPresentationStyle,
		AActor* InSourceBoss,
		AActor* InInstigatorActor,
		const FGPBossDeathPresentationSpawnSettings& InSettings);

	UFUNCTION(BlueprintPure, Category = "Boss|Death Presentation")
	EGPBossDeathPresentationStyle GetPresentationStyle() const { return PresentationStyle; }

	UFUNCTION(BlueprintPure, Category = "Boss|Death Presentation")
	int32 GetSpawnedPieceCount() const;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Death Presentation")
	void BP_OnPresentationStarted(EGPBossDeathPresentationStyle StartedStyle, AActor* SourceBoss, AActor* InstigatorActor);

private:
	struct FPieceMotion
	{
		TWeakObjectPtr<USceneComponent> Component;
		FVector Velocity = FVector::ZeroVector;
		FRotator AngularVelocity = FRotator::ZeroRotator;
		float GravityScale = 0.0f;
		float StartDelaySeconds = 0.0f;
		float HideAtSeconds = 3.0f;
		FVector InitialScale = FVector::OneVector;
		FVector TargetScale = FVector::OneVector;
		bool bShrinkOverLifetime = false;
		bool bStarted = false;
	};

	void StartPresentation();
	void StartCrystalSeraphPresentation();
	void StartSansPresentation();
	void StartDarkArmorKnightPresentation();
	void StartMatadorPresentation();
	void SpawnBurstNiagara(UNiagaraSystem* NiagaraSystem, const FVector& Offset, const FVector& Scale);
	void HideSourceMesh();
	void AddStaticPiece(
		UStaticMesh* Mesh,
		const FVector& WorldLocation,
		const FRotator& WorldRotation,
		const FVector& WorldScale,
		const FVector& Velocity,
		const FRotator& AngularVelocity,
		float GravityScale,
		float HideAtSeconds,
		const FLinearColor& Tint,
		float StartDelaySeconds = 0.0f,
		bool bShrinkOverLifetime = false,
		const FVector& TargetScale = FVector::ZeroVector);
	void AddSkeletalPiece(
		USkeletalMesh* Mesh,
		UAnimationAsset* Animation,
		const FVector& WorldLocation,
		const FRotator& WorldRotation,
		const FVector& WorldScale,
		const FVector& Velocity,
		const FRotator& AngularVelocity,
		float GravityScale,
		float HideAtSeconds,
		const FLinearColor& Tint,
		float StartDelaySeconds = 0.0f,
		bool bShrinkOverLifetime = false,
		const FVector& TargetScale = FVector::ZeroVector);
	UMaterialInterface* CreateTintMaterial(const FLinearColor& Tint);
	FVector ResolveGroundLocation(const FVector& DesiredLocation) const;
	FVector RandomHorizontalDirection();
	float GetScaled(float Value) const;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Death Presentation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Death Presentation|Meshes")
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Death Presentation|Meshes")
	TObjectPtr<UStaticMesh> ConeMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Death Presentation|Meshes")
	TObjectPtr<UStaticMesh> CylinderMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Death Presentation|Meshes")
	TObjectPtr<USkeletalMesh> SansHandMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Death Presentation|Meshes")
	TObjectPtr<USkeletalMesh> BullMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Death Presentation|Meshes")
	TObjectPtr<UAnimationAsset> BullRunAnimation;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Death Presentation|Materials")
	TObjectPtr<UMaterialInterface> BasicShapeMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Death Presentation|VFX")
	TObjectPtr<UNiagaraSystem> DefaultDarkLightningSystem;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> StaticPieces;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USkeletalMeshComponent>> SkeletalPieces;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DynamicMaterials;

	TArray<FPieceMotion> PieceMotions;
	TWeakObjectPtr<AActor> SourceBossActor;
	TWeakObjectPtr<AActor> PresentationInstigatorActor;
	FGPBossDeathPresentationSpawnSettings Settings;
	EGPBossDeathPresentationStyle PresentationStyle = EGPBossDeathPresentationStyle::None;
	FRandomStream RandomStream;
	FTimerHandle SourceMeshHideTimerHandle;
	float ElapsedSeconds = 0.0f;
};
