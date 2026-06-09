#pragma once

#include "CoreMinimal.h"
#include "Actors/GP_AreaProjectile.h"
#include "TimerManager.h"
#include "GP_GroundLaunchProjectile.generated.h"

UCLASS()
class PROJECT_EDEN_API AGP_GroundLaunchProjectile : public AGP_AreaProjectile
{
	GENERATED_BODY()

public:
	AGP_GroundLaunchProjectile();

	void InitializeGroundLaunch(
		const FVector& GroundLocation,
		const FVector& LaunchDirection,
		float RiseDepth,
		float RiseHeight,
		float RiseDuration,
		float LaunchDelay,
		float LaunchSpeed);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void LaunchProjectile();

	FVector RiseStartLocation = FVector::ZeroVector;
	FVector RiseEndLocation = FVector::ZeroVector;
	FVector PendingLaunchDirection = FVector::ForwardVector;
	float PendingRiseDuration = 0.4f;
	float PendingLaunchDelay = 0.25f;
	float PendingLaunchSpeed = 900.f;
	float RiseElapsedTime = 0.f;
	FTimerHandle LaunchDelayTimerHandle;
	bool bGroundLaunchInitialized = false;
	bool bWaitingToLaunch = false;
	bool bHasLaunched = false;
};
