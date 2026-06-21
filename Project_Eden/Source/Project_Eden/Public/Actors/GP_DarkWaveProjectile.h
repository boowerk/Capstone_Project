#pragma once

#include "CoreMinimal.h"
#include "Actors/GP_Projectile.h"
#include "GP_DarkWaveProjectile.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

/** Prototype sword-wave projectile. The cube mesh/component can be replaced on a Blueprint child later. */
UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_DarkWaveProjectile : public AGP_Projectile
{
	GENERATED_BODY()

public:
	AGP_DarkWaveProjectile();

protected:
	virtual void OnProjectileOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> WaveCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> WaveMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float AttackPowerDamageCoefficient = 1.0f;
};
