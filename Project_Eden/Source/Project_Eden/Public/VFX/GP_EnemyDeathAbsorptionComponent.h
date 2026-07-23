#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "GP_EnemyDeathAbsorptionComponent.generated.h"

class AActor;
class AGP_PlayerCharacter;
class UNiagaraComponent;
class UNiagaraSystem;
class USkeletalMeshComponent;

/**
 * Converts a regular enemy's final pose into Niagara particles and attracts
 * those particles toward the one player selected by the authoritative death.
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
	void HideSourceMeshWhenReady();
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

	// This duplicate is authored from NS_Simple_SK; a missing optional asset leaves the enemy mesh untouched.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption",
		meta = (AllowPrivateAccess = "true"))
	TSoftObjectPtr<UNiagaraSystem> DeathAbsorptionSystem;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption|Target",
		meta = (AllowPrivateAccess = "true"))
	FName TargetBodySocketName = TEXT("spine_03");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption|Target",
		meta = (AllowPrivateAccess = "true", Units = "cm"))
	FVector TargetBodyOffset = FVector::ZeroVector;

	// A short scatter phase preserves the recognizable enemy silhouette before particles accelerate inward.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption|Timing",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float ScatterDelaySeconds = 0.20f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption|Timing",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.01", Units = "s"))
	float AbsorbStrengthRampSeconds = 0.55f;

	// The mesh stays visible for the first Niagara sampling frames, then the particles become the only corpse silhouette.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption|Timing",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float SourceMeshHideDelaySeconds = 0.08f;

	// Compress the original five-second dissolve curves into the regular enemy's two-second corpse window.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption|Timing",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.1"))
	float NiagaraPlaybackRate = 3.0f;

	// With 3x Niagara time, this window limits the original 25k/s rate to roughly 4.5k particles per corpse.
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
	float EffectDeactivateTimeSeconds = 1.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption|Force",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float MaximumAbsorbStrength = 9000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption|Force",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float MinimumAbsorbRadius = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death Absorption|Force",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float AbsorbKillRadius = 28.0f;

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

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> ActiveNiagaraComponent;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> SourceMeshComponent;

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
