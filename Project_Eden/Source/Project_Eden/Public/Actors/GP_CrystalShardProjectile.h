#pragma once

#include "CoreMinimal.h"
#include "Actors/GP_Projectile.h"
#include "GP_CrystalShardProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UGP_VisualCueComponent;

UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_CrystalShardProjectile : public AGP_Projectile
{
	GENERATED_BODY()

public:
	AGP_CrystalShardProjectile();
	virtual void BeginPlay() override;

protected:
	virtual void OnProjectileOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

private:
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayShardImpactVFX(const FVector& ImpactLocation, const FRotator& ImpactRotation);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph|VFX", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGP_VisualCueComponent> VisualCueComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> ShardCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> ShardMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float AttackPowerDamageCoefficient = 0.8f;
};
