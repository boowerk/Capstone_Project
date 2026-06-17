#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "GP_ProjectileVFXSpawn.generated.h"

class AGP_ProjectileMaster;
class UNiagaraSystem;

UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_ProjectileVFXSpawn : public AActor
{
	GENERATED_BODY()

public:
	AGP_ProjectileVFXSpawn();

	UFUNCTION(BlueprintCallable, Category = "Eden|Projectile|VFX")
	void SpawnProjectileVFX();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eden|Projectile|VFX")
	TSubclassOf<AGP_ProjectileMaster> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eden|Projectile|VFX")
	TObjectPtr<UNiagaraSystem> MuzzleVFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eden|Projectile|VFX")
	TObjectPtr<UNiagaraSystem> MainProjectileVFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eden|Projectile|VFX")
	TObjectPtr<UNiagaraSystem> HitImpactVFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eden|Projectile|VFX")
	float ProjectileSpeed = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eden|Projectile|VFX")
	float ProjectileSpawnGap = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eden|Projectile|VFX")
	float ProjectileDestroyDelay = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eden|Projectile|VFX")
	FVector MuzzleHitImpactSpawnOffset = FVector(0.f, 0.f, 100.f);

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Eden|Projectile|VFX")
	TObjectPtr<AGP_ProjectileMaster> SpawnedProjectile;

private:
	FTimerHandle SpawnProjectileTimerHandle;
};
