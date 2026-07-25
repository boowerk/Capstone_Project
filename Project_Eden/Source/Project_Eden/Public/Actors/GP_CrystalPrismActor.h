#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP_CrystalPrismActor.generated.h"

class AGP_CrystalSeraphBossCharacter;
class AGP_SeraphLaserActor;
class UGP_VisualCueComponent;
class UBoxComponent;
class UMaterialInstanceDynamic;
class UNiagaraComponent;
class UNiagaraSystem;
class USphereComponent;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_CrystalPrismActor : public AActor
{
	GENERATED_BODY()

public:
	AGP_CrystalPrismActor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Boss|Crystal Seraph")
	void InitializePrism(AGP_CrystalSeraphBossCharacter* InBossOwner);

	UFUNCTION(BlueprintCallable, Category = "Boss|Crystal Seraph")
	bool NotifyLaserHit(AGP_SeraphLaserActor* LaserActor, const FVector& IncomingDirection);

	/** Native laser path supplies the actual shield-surface contact for reflection VFX. */
	bool NotifyLaserHitAtLocation(AGP_SeraphLaserActor* LaserActor, const FVector& IncomingDirection, const FVector& ReflectionLocation);

	UFUNCTION(BlueprintPure, Category = "Boss|Crystal Seraph")
	FVector ResolveReflectedDirection(const FVector& IncomingDirection) const;

	UFUNCTION(BlueprintPure, Category = "Boss|Crystal Seraph")
	float GetCollisionRadius() const;

	/** Shared visual/gameplay center for the shield and laser reflection. */
	UFUNCTION(BlueprintPure, Category = "Boss|Crystal Seraph|Shield")
	FVector GetReflectionCenter() const;

	/** Starts the short shield break sequence. The boss owns the pattern timing; the prism owns its visual. */
	UFUNCTION(BlueprintCallable, Category = "Boss|Crystal Seraph|Shield")
	void BeginPrismShieldRelease();

	UFUNCTION(BlueprintPure, Category = "Boss|Crystal Seraph|VFX")
	FVector GetPrismAuraScale() const { return PrismAuraScale; }

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Crystal Seraph")
	void BP_OnLaserReflected(AGP_SeraphLaserActor* LaserActor, const FVector& ReflectedDirection);

private:
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayReflectionVFX(const FVector& ReflectionLocation, const FRotator& ReflectionRotation);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastBeginPrismShieldRelease();

	UFUNCTION()
	void OnRep_BossOwner();

	void ActivatePrismShield();
	void UpdatePrismShieldVisual(float DeltaSeconds);
	void UpdatePrismShieldDirection();
	FVector GetPrismShieldCenterOffset() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph|VFX", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGP_VisualCueComponent> VisualCueComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph|VFX", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGP_VisualCueComponent> PrismShieldVFXComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> PrismMesh;

	/** Solid collision for the crystal body only; the large shield sphere remains query-only and IK-safe. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> PrismBodyCollision;

	/** Assigned directly in BP_CrystalPrism with the shield material; this is the primary shield renderer. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph|Shield", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> PrismShieldMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> ReflectionCollision;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float PrismLifeSpan = 14.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "180.0", Units = "deg"))
	float ReflectionAngleDegrees = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float CollisionRadius = 250.0f;

	// Keep the native fallback neutral; Blueprint children own mesh-specific visual scale.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true"))
	FVector PrismVisualScale = FVector::OneVector;

	// Aura scale is independent from the enlarged mesh so designers can keep the persistent effect close to the crystal.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph|VFX", meta = (AllowPrivateAccess = "true"))
	FVector PrismAuraScale = FVector(0.55f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Crystal Seraph|Visual", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMesh> PrismStaticMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Crystal Seraph|VFX", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraSystem> PrismAuraVFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Crystal Seraph|VFX", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraSystem> PrismReflectionVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph|VFX", meta = (AllowPrivateAccess = "true"))
	FVector PrismReflectionVFXScale = FVector(1.35f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Crystal Seraph|Shield", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraSystem> PrismShieldVFX;

	/** Optional local adjustment added after the shield is centered on the prism body bounds. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph|Shield", meta = (AllowPrivateAccess = "true"))
	FVector PrismShieldRelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph|Shield", meta = (AllowPrivateAccess = "true"))
	// Engine Sphere has a 50cm base radius; 5x matches the 250cm reflection collision radius.
	FVector PrismShieldVisualScale = FVector(5.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph|Shield", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float PrismShieldFadeInDuration = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph|Shield", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float PrismShieldReleaseDuration = 0.3f;

	UPROPERTY(ReplicatedUsing = OnRep_BossOwner, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AGP_CrystalSeraphBossCharacter> BossOwner;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PrismShieldMaterialInstance;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> ActivePrismShieldVFX;

	bool bPrismShieldFadingIn = false;
	bool bPrismShieldReleasing = false;
	float PrismShieldPhaseElapsed = 0.0f;
};
