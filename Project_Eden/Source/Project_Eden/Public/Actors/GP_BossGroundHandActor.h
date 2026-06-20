#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP_BossGroundHandActor.generated.h"

class UBoxComponent;
class UDecalComponent;
class UGameplayEffect;
class UMaterialInterface;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

USTRUCT()
struct FGPBossGroundHandPatternSpec
{
	GENERATED_BODY()

	UPROPERTY()
	float TelegraphDuration = 0.75f;

	UPROPERTY()
	float RiseDuration = 0.22f;

	UPROPERTY()
	float HoldDuration = 0.35f;

	UPROPERTY()
	float RetractDuration = 0.28f;

	UPROPERTY()
	bool bInitialized = false;
};

UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_BossGroundHandActor : public AActor
{
	GENERATED_BODY()

public:
	AGP_BossGroundHandActor();

	// Starts the warning-to-rise sequence after the ability has positioned this actor near the player.
	void InitializeGroundHand(float InTelegraphDuration);

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_PatternSpec();

	UFUNCTION()
	void OnHandOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void ApplyPatternStart();
	void UpdatePatternVisuals(float ElapsedSeconds);
	void SnapToFloor();
	void ApplyDamageAndLaunch(AActor* TargetActor);
	void ConfigureMeshComponent(UStaticMeshComponent* MeshComponent, UStaticMesh* MeshAsset) const;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Ground Hands", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Ground Hands", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> HandVisualRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Ground Hands", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UDecalComponent> WarningDecal;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Ground Hands", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> HandCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Ground Hands", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> PalmMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Ground Hands", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> WristMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Ground Hands", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UStaticMeshComponent>> FingerMeshes;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Ground Hands|Telegraph")
	TObjectPtr<UMaterialInterface> WarningDecalMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Ground Hands|Telegraph", meta = (ClampMin = "10.0", Units = "cm"))
	float WarningRadius = 115.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Ground Hands|Motion", meta = (ClampMin = "0.01", Units = "s"))
	float RiseDuration = 0.22f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Ground Hands|Motion", meta = (ClampMin = "0.0", Units = "s"))
	float HoldDuration = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Ground Hands|Motion", meta = (ClampMin = "0.01", Units = "s"))
	float RetractDuration = 0.28f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Ground Hands|Motion", meta = (ClampMin = "0.0", Units = "cm"))
	float HiddenDepth = 240.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Ground Hands|Damage", meta = (ClampMin = "0.0"))
	float BaseDamage = 14.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Ground Hands|Damage", meta = (ClampMin = "0.0"))
	float AttackPowerDamageCoefficient = 0.75f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Ground Hands|Damage", meta = (ClampMin = "0.0", Units = "cm/s"))
	float LaunchSpeed = 1250.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Ground Hands|Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Ground Hands|Placement", meta = (ClampMin = "0.0", Units = "cm"))
	float FloorTraceUpDistance = 400.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Ground Hands|Placement", meta = (ClampMin = "0.0", Units = "cm"))
	float FloorTraceDownDistance = 2000.0f;

	UPROPERTY(ReplicatedUsing = OnRep_PatternSpec)
	FGPBossGroundHandPatternSpec PatternSpec;

	float LocalElapsedSeconds = 0.0f;
	bool bPatternStarted = false;
	bool bCollisionActive = false;
	TSet<TWeakObjectPtr<AActor>> HitActors;
};
