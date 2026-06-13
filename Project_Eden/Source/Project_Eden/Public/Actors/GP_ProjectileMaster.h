#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP_ProjectileMaster.generated.h"

class UBoxComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UProjectileMovementComponent;
class UPrimitiveComponent;

UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_ProjectileMaster : public AActor
{
	GENERATED_BODY()

public:
	AGP_ProjectileMaster();

	UFUNCTION(BlueprintCallable, Category = "Eden|Projectile")
	void SpawnVFX(UNiagaraSystem* VFXToSpawn, FVector SpawnOffset);

	UFUNCTION(BlueprintCallable, Category = "Eden|Projectile")
	void ConfigureProjectileVFX(
		UNiagaraSystem* InMuzzleVFX,
		UNiagaraSystem* InMainProjectileVFX,
		UNiagaraSystem* InHitImpactVFX,
		float InProjectileDestroyDelay,
		FVector InMuzzleHitImpactSpawnOffset,
		FVector InitialVelocity);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Eden|Projectile")
	TObjectPtr<UBoxComponent> ProjectileCollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Eden|Projectile")
	TObjectPtr<UNiagaraComponent> ProjectileVFXComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Eden|Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileVFXMovementComp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Eden|Projectile|VFX")
	TObjectPtr<UNiagaraSystem> MuzzleVFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Eden|Projectile|VFX")
	TObjectPtr<UNiagaraSystem> HitImpactVFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Eden|Projectile")
	float ProjectileDestroyDelay = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Eden|Projectile")
	FVector MuzzleHitImpactSpawnOffset = FVector::ZeroVector;

	UFUNCTION()
	void OnProjectileCollisionBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	void DestroyAfterImpact();

	void SpawnMuzzleVFX();
};
