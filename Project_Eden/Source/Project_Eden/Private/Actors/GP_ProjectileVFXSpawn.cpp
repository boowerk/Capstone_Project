#include "Actors/GP_ProjectileVFXSpawn.h"

#include "Actors/GP_ProjectileMaster.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AGP_ProjectileVFXSpawn::AGP_ProjectileVFXSpawn()
{
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FClassFinder<AGP_ProjectileMaster> ProjectileClassFinder(TEXT("/Game/Niagara/Slash/BP_ProjectileMaster"));
	if (ProjectileClassFinder.Succeeded())
	{
		ProjectileClass = ProjectileClassFinder.Class;
	}
	else
	{
		ProjectileClass = AGP_ProjectileMaster::StaticClass();
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> MainProjectileVFXFinder(TEXT("/Game/Niagara/Slash/NS_PlaySlash.NS_PlaySlash"));
	if (MainProjectileVFXFinder.Succeeded())
	{
		MainProjectileVFX = MainProjectileVFXFinder.Object;
	}
}

void AGP_ProjectileVFXSpawn::BeginPlay()
{
	Super::BeginPlay();

	SpawnProjectileVFX();

	if (ProjectileSpawnGap > 0.f)
	{
		GetWorldTimerManager().SetTimer(
			SpawnProjectileTimerHandle,
			this,
			&ThisClass::SpawnProjectileVFX,
			ProjectileSpawnGap,
			true);
	}
}

void AGP_ProjectileVFXSpawn::SpawnProjectileVFX()
{
	if (!ProjectileClass)
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;

	SpawnedProjectile = GetWorld()->SpawnActor<AGP_ProjectileMaster>(
		ProjectileClass,
		GetActorLocation(),
		GetActorRotation(),
		SpawnParameters);

	if (!IsValid(SpawnedProjectile))
	{
		return;
	}

	SpawnedProjectile->ConfigureProjectileVFX(
		MuzzleVFX,
		MainProjectileVFX,
		HitImpactVFX,
		ProjectileDestroyDelay,
		MuzzleHitImpactSpawnOffset,
		GetActorForwardVector() * ProjectileSpeed);
}
