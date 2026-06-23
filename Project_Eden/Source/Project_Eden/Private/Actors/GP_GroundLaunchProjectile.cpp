#include "Actors/GP_GroundLaunchProjectile.h"

#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

AGP_GroundLaunchProjectile::AGP_GroundLaunchProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	if (ProjectileMovement)
	{
		ProjectileMovement->ProjectileGravityScale = 0.f;
	}
}

void AGP_GroundLaunchProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Deactivate();
	}

	if (CollisionComponent)
	{
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AGP_GroundLaunchProjectile::InitializeGroundLaunch(
	const FVector& GroundLocation,
	const FVector& LaunchDirection,
	float RiseDepth,
	float RiseHeight,
	float RiseDuration,
	float LaunchDelay,
	float LaunchSpeed)
{
	if (!HasAuthority())
	{
		return;
	}

	const float SafeRiseDepth = FMath::Max(0.f, RiseDepth);
	const float SafeRiseHeight = FMath::Max(0.f, RiseHeight);

	RiseStartLocation = GroundLocation - FVector::UpVector * SafeRiseDepth;
	RiseEndLocation = GroundLocation + FVector::UpVector * SafeRiseHeight;
	PendingLaunchDirection = LaunchDirection.GetSafeNormal();
	if (PendingLaunchDirection.IsNearlyZero())
	{
		PendingLaunchDirection = GetActorForwardVector();
	}

	PendingRiseDuration = FMath::Max(0.01f, RiseDuration);
	PendingLaunchDelay = FMath::Max(0.f, LaunchDelay);
	PendingLaunchSpeed = FMath::Max(0.f, LaunchSpeed);
	RiseElapsedTime = 0.f;
	bGroundLaunchInitialized = true;
	bWaitingToLaunch = false;
	bHasLaunched = false;

	SetActorLocation(RiseStartLocation);
	SetActorRotation(PendingLaunchDirection.Rotation());
	SetActorTickEnabled(true);
	ForceNetUpdate();
}

void AGP_GroundLaunchProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority() || !bGroundLaunchInitialized || bWaitingToLaunch || bHasLaunched)
	{
		return;
	}

	RiseElapsedTime += DeltaSeconds;
	const float Alpha = FMath::Clamp(RiseElapsedTime / PendingRiseDuration, 0.f, 1.f);
	const float SmoothedAlpha = FMath::SmoothStep(0.f, 1.f, Alpha);
	SetActorLocation(FMath::Lerp(RiseStartLocation, RiseEndLocation, SmoothedAlpha));

	if (Alpha >= 1.f)
	{
		SetActorTickEnabled(false);

		if (PendingLaunchDelay <= 0.f)
		{
			LaunchProjectile();
			return;
		}

		bWaitingToLaunch = true;
		GetWorldTimerManager().SetTimer(
			LaunchDelayTimerHandle,
			this,
			&ThisClass::LaunchProjectile,
			PendingLaunchDelay,
			false);
	}
}

void AGP_GroundLaunchProjectile::LaunchProjectile()
{
	bWaitingToLaunch = false;
	bHasLaunched = true;
	SetActorTickEnabled(false);

	if (CollisionComponent)
	{
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}

	if (ProjectileMovement)
	{
		ProjectileMovement->Velocity = PendingLaunchDirection * PendingLaunchSpeed;
		ProjectileMovement->Activate(true);
	}

	ForceNetUpdate();
}
