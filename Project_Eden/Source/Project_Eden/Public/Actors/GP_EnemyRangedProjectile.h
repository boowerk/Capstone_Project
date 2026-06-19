#pragma once

#include "CoreMinimal.h"
#include "Actors/GP_Projectile.h"
#include "GP_EnemyRangedProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_EnemyRangedProjectile : public AGP_Projectile
{
	GENERATED_BODY()

public:
	AGP_EnemyRangedProjectile();

	// Applies an explicit world-space velocity so the projectile always follows the resolved player aim direction.
	void LaunchToward(const FVector& WorldDirection);

protected:
	virtual void OnProjectileOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Ranged", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> ProjectileCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Ranged", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Ranged|Damage", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float BaseDamage = 8.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Ranged|Damage", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float AttackPowerDamageCoefficient = 0.65f;
};
