#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "GP_EnemyDeathAbsorptionComponent.generated.h"

class AActor;
class AGP_PlayerCharacter;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UMeshComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class USkeletalMeshComponent;

/**
 * Converts an enemy's final pose into Niagara particles and attracts those
 * particles toward the one player selected by the authoritative death.
 *
 * The component also owns the source-mesh material swap/dissolve lifecycle.
 * Boss-specific presentation actors remain independent and layer their
 * bespoke shards/particles on top of this shared death treatment.
 */
UCLASS(ClassGroup = (Enemy), meta = (BlueprintSpawnableComponent, DisplayName = "Enemy Death Absorption VFX"))
class PROJECT_EDEN_API UGP_EnemyDeathAbsorptionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGP_EnemyDeathAbsorptionComponent();

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	/**
	 * Authority-only entry point. The target is resolved once here so all three
	 * players see the same corpse travel toward the same player.
	 */
	void PlayDeathAbsorption(AActor* DeathInstigatorActor);

	UFUNCTION(BlueprintPure, Category = "Enemy|Death Absorption")
	UNiagaraSystem* GetDeathAbsorptionSystem() const;

	UFUNCTION(BlueprintPure, Category = "Enemy|Death Absorption")
	bool IsDeathAbsorptionActive() const { return bLocalPlaybackActive; }

	UFUNCTION(BlueprintCallable, Category = "Enemy|Death Absorption|Materials")
	void ConfigureDeathMaterials(
		UMaterialInterface* InDefaultMaterial,
		const TArray<UMaterialInterface*>& InSlotMaterialOverrides,
		UMaterialInterface* InParticleMaterial);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Death Absorption")
	void SetDeathAbsorptionEnabled(bool bEnabled) { bEnableDeathAbsorption = bEnabled; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Death Absorption")
	bool IsDeathAbsorptionEnabled() const { return bEnableDeathAbsorption; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Death Absorption|Materials")
	UMaterialInterface* GetDefaultDeathDissolveMaterial() const { return DefaultDeathDissolveMaterial; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Death Absorption|Materials")
	UMaterialInterface* GetDeathParticleMaterial() const { return DeathParticleMaterial; }

	const TArray<TObjectPtr<UMaterialInterface>>& GetDeathDissolveMaterialOverrides() const
	{
		return DeathDissolveMaterialOverrides;
	}

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayDeathAbsorption(AActor* TargetPlayerActor);

	void PlayLocal(AActor* TargetPlayerActor);
	void StopLocalPlayback();
	AActor* ResolveAuthorityTarget(AActor* DeathInstigatorActor) const;
	AGP_PlayerCharacter* ResolvePlayerCharacter(AActor* CandidateActor) const;
	bool IsUsablePlayerCharacter(const AGP_PlayerCharacter* PlayerCharacter) const;
	FVector ResolveTargetPosition(const AActor* TargetActor) const;
	void UpdateTargetAndBounds();
	void UpdateAbsorbStrength();
	void UpdateFallGravity();
	void ApplyDeathDissolveMaterials();
	void UpdateDeathDissolveMaterials();
	void HideSourceMeshesWhenReady();
	void UpdateCorridorFixedBounds();

	// The policy helpers are kept side-effect free so target priority and timing can be covered without a live server.
	static AActor* SelectPreferredOrNearestTarget(
		AActor* PreferredTarget,
		const FVector& SourceLocation,
		const TArray<AActor*>& ValidFallbackTargets);
	static float CalculateAbsorbStrength(
		float ElapsedSeconds,
		float ScatterDelaySeconds,
		float RampDurationSeconds,
		float MaximumStrength);
	static float CalculateFallGravityScale(
		float ElapsedSeconds,
		float FullGravityDurationSeconds,
		float GravityFadeEndSeconds);

	// This duplicate is authored from NS_Simple_SK; a missing optional asset leaves the enemy mesh untouched.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption",
		meta = (AllowPrivateAccess = "true"))
	TSoftObjectPtr<UNiagaraSystem> DeathAbsorptionSystem;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption",
		meta = (AllowPrivateAccess = "true"))
	bool bEnableDeathAbsorption = true;

	// Used for every visible mesh component and as the fallback for unconfigured material slots.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption|Materials",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> DefaultDeathDissolveMaterial;

	// Main CharacterMesh0 slot overrides. Auxiliary wings/weapons use the default material.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption|Materials",
		meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UMaterialInterface>> DeathDissolveMaterialOverrides;

	// Per-enemy Niagara Sprite Renderer material supplied through User.DeathParticleMaterial.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption|Materials",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> DeathParticleMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption|Target",
		meta = (AllowPrivateAccess = "true"))
	FName TargetBodySocketName = TEXT("spine_03");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption|Target",
		meta = (AllowPrivateAccess = "true", Units = "cm"))
	FVector TargetBodyOffset = FVector::ZeroVector;

	// Attraction waits while the particles begin falling, then overlaps the fading gravity to create a curved path.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption|Timing",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float ScatterDelaySeconds = 0.38f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption|Timing",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.01", Units = "s"))
	float AbsorbStrengthRampSeconds = 0.80f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption|Timing",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float FullGravityDurationSeconds = 0.28f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption|Timing",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float GravityFadeEndSeconds = 0.60f;

	// The replacement material visibly dissolves for this duration. Niagara has
	// already sampled several frames before every source visual is hidden.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption|Timing",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float SourceMeshHideDelaySeconds = 0.45f;

	// Keep the authored five-second curves close to completion without recreating the previous overly fast 3x motion.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption|Timing",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.1"))
	float NiagaraPlaybackRate = 2.6f;

	// At 2.6x Niagara time, this limits the original 25k/s source to roughly 3.9k grains at steady frame pacing.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption|Timing",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.01", Units = "s"))
	float EmissionStopTimeSeconds = 0.06f;

	// A hitch must not deactivate the system before Niagara has received enough update frames to emit a visible corpse.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption|Timing",
		meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 MinimumEmissionFrames = 3;

	// The default enemy corpse is destroyed at two seconds, so finish the complete effect before that boundary.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption|Timing",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.1", Units = "s"))
	float EffectDeactivateTimeSeconds = 1.90f;

	// Constant-acceleration attraction is intentionally much lower than the old distance-scaled spring strength.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption|Force",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float MaximumAbsorbStrength = 800.0f;

	// Niagara playback dilation amplifies acceleration, so this value produces a readable fall rather than a hard drop.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption|Force",
		meta = (AllowPrivateAccess = "true"))
	FVector FallGravity = FVector(0.0f, 0.0f, -160.0f);

	// Drag prevents the constant attraction from rebuilding the previous high-speed snap near the player.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption|Force",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float AbsorbDrag = 1.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption|Force",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float MinimumAbsorbRadius = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption|Force",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float AbsorbKillRadius = 45.0f;

	// GPU particles need a fixed bounds corridor that covers both the corpse and a moving target.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption|Bounds",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float BoundsMargin = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption|Parameters",
		meta = (AllowPrivateAccess = "true"))
	FName SourceMeshParameterName = TEXT("User.SourceMesh");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption|Parameters",
		meta = (AllowPrivateAccess = "true"))
	FName AbsorbTargetPositionParameterName = TEXT("User.AbsorbTargetPosition");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption|Parameters",
		meta = (AllowPrivateAccess = "true"))
	FName AbsorbStrengthParameterName = TEXT("User.AbsorbStrength");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption|Parameters",
		meta = (AllowPrivateAccess = "true"))
	FName AbsorbRadiusParameterName = TEXT("User.AbsorbRadius");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption|Parameters",
		meta = (AllowPrivateAccess = "true"))
	FName AbsorbKillRadiusParameterName = TEXT("User.AbsorbKillRadius");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption|Parameters",
		meta = (AllowPrivateAccess = "true"))
	FName FallGravityParameterName = TEXT("User.FallGravity");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption|Parameters",
		meta = (AllowPrivateAccess = "true"))
	FName AbsorbDragParameterName = TEXT("User.AbsorbDrag");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption|Parameters",
		meta = (AllowPrivateAccess = "true"))
	FName DeathParticleMaterialParameterName = TEXT("User.DeathParticleMaterial");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption|Parameters",
		meta = (AllowPrivateAccess = "true"))
	FName DissolveProgressParameterName = TEXT("DissolveProgress");

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> ActiveNiagaraComponent;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> SourceMeshComponent;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMeshComponent>> SourceVisualMeshComponents;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> ActiveDissolveMaterials;

	TWeakObjectPtr<AActor> ActiveTargetActor;
	FBox CachedSourceWorldBounds = FBox(EForceInit::ForceInit);
	FVector LastTargetPosition = FVector::ZeroVector;
	float PlaybackElapsedSeconds = 0.0f;
	int32 PlaybackTickCount = 0;
	bool bAuthorityPlaybackRequested = false;
	bool bLocalPlaybackActive = false;
	bool bEmissionStopped = false;
	bool bSourceMeshHiddenByComponent = false;

#if WITH_DEV_AUTOMATION_TESTS
	friend class FEnemyDeathAbsorptionPolicyTest;
#endif
};
