#pragma once

#include "CoreMinimal.h"
#include "Actors/GP_Projectile.h"
#include "TimerManager.h"
#include "GP_MultiHitProjectile.generated.h"

UCLASS()
class PROJECT_EDEN_API AGP_MultiHitProjectile : public AGP_Projectile
{
	GENERATED_BODY()

public:
	AGP_MultiHitProjectile();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnProjectileOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult) override;

private:
	UFUNCTION()
	void OnMultiHitProjectileHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit);

	void ApplyNextHit();
	void StopHitTimer();

	TArray<TWeakObjectPtr<AActor>> HitCandidates;
	FTimerHandle HitTimerHandle;
	int32 TotalAppliedHits = 0;
	int32 NextCandidateIndex = 0;
};
