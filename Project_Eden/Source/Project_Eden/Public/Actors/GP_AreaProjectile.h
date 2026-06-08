#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "GP_AreaProjectile.generated.h"

class UGameplayEffect;
class UGP_SkillData;
class UPrimitiveComponent;
class UProjectileMovementComponent;
class USphereComponent;
class UNiagaraSystem;

UCLASS()
class PROJECT_EDEN_API AGP_AreaProjectile : public AActor
{
	GENERATED_BODY()

public:
	AGP_AreaProjectile();

	void SetSkillData(UGP_SkillData* InSkillData) { SkillData = InSkillData; }
	void SetImpactVisualActorClass(TSubclassOf<AActor> InImpactVisualActorClass) { ImpactVisualActorClass = InImpactVisualActorClass; }
	void SetProjectileVisualSystem(UNiagaraSystem* InProjectileVisualSystem);
	void ApplyExplosionRadiusMultiplier(float RadiusMultiplier);

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Eden|Projectile")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Eden|Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Eden|GAS")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Eden|GAS")
	FGameplayTag HitEventTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Eden|GAS", meta = (ClampMin = "0.0"))
	float ExplosionRadius = 300.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Eden|Visuals")
	TSubclassOf<AActor> ImpactVisualActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Eden|Projectile")
	float ProjectileLifeSpan = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Eden|Debug")
	bool bDrawDebug = false;

	UPROPERTY(BlueprintReadWrite, Category = "Eden|GAS", meta = (ExposeOnSpawn = "true"))
	float EffectLevel = 1.f;

	UPROPERTY(BlueprintReadWrite, Category = "Eden|GAS", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<UGP_SkillData> SkillData;

	UPROPERTY(ReplicatedUsing = OnRep_ProjectileVisualSystem, BlueprintReadOnly, Category = "Eden|Projectile|Visual")
	TObjectPtr<UNiagaraSystem> ProjectileVisualSystem;

	UFUNCTION()
	void OnRep_ProjectileVisualSystem();

	UFUNCTION(BlueprintImplementableEvent, Category = "Eden|Projectile|Visual")
	void BP_OnProjectileVisualSystemChanged(UNiagaraSystem* NewProjectileVisualSystem);

	UFUNCTION()
	void OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void OnProjectileOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpawnImpactVisual(const FVector& ImpactLocation, TSubclassOf<AActor> VisualActorClass, float VisualScale);

	void Explode(const FVector& ImpactLocation);

	UPROPERTY(Transient)
	bool bHasExploded = false;

	UPROPERTY(Transient)
	float VisualScaleMultiplier = 1.0f;
};
