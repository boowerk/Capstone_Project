#include "Actors/GP_ProjectileMaster.h"

#include "Components/BoxComponent.h"
#include "Engine/BlockingVolume.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AGP_ProjectileMaster::AGP_ProjectileMaster()
{
	PrimaryActorTick.bCanEverTick = false;

	ProjectileCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ProjectileCollisionBox"));
	SetRootComponent(ProjectileCollisionBox);
	ProjectileCollisionBox->SetGenerateOverlapEvents(true);
	ProjectileCollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ProjectileCollisionBox->SetCollisionObjectType(ECC_WorldDynamic);
	ProjectileCollisionBox->SetCollisionResponseToAllChannels(ECR_Overlap);

	ProjectileVFXComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ProjectileVFXComponent"));
	ProjectileVFXComponent->SetupAttachment(ProjectileCollisionBox);
	ProjectileVFXComponent->SetAutoActivate(true);
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> ProjectileVFXSystemFinder(TEXT("/Game/Niagara/Slash/NS_PlaySlash.NS_PlaySlash"));
	if (ProjectileVFXSystemFinder.Succeeded())
	{
		ProjectileVFXComponent->SetAsset(ProjectileVFXSystemFinder.Object);
	}

	ProjectileVFXMovementComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileVFXMovementComp"));
	ProjectileVFXMovementComp->UpdatedComponent = ProjectileCollisionBox;
	ProjectileVFXMovementComp->ProjectileGravityScale = 0.0f;
}

void AGP_ProjectileMaster::BeginPlay()
{
	Super::BeginPlay();

	if (ProjectileCollisionBox)
	{
		ProjectileCollisionBox->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnProjectileCollisionBeginOverlap);
	}

	FTimerHandle MuzzleVFXTimerHandle;
	GetWorldTimerManager().SetTimer(
		MuzzleVFXTimerHandle,
		this,
		&ThisClass::SpawnMuzzleVFX,
		0.1f,
		false);
}

void AGP_ProjectileMaster::SpawnVFX(UNiagaraSystem* VFXToSpawn, FVector SpawnOffset)
{
	if (!IsValid(VFXToSpawn))
	{
		return;
	}

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, VFXToSpawn, GetActorLocation() + SpawnOffset);
}

void AGP_ProjectileMaster::ConfigureProjectileVFX(
	UNiagaraSystem* InMuzzleVFX,
	UNiagaraSystem* InMainProjectileVFX,
	UNiagaraSystem* InHitImpactVFX,
	float InProjectileDestroyDelay,
	FVector InMuzzleHitImpactSpawnOffset,
	FVector InitialVelocity)
{
	MuzzleVFX = InMuzzleVFX;
	HitImpactVFX = InHitImpactVFX;
	ProjectileDestroyDelay = InProjectileDestroyDelay;
	MuzzleHitImpactSpawnOffset = InMuzzleHitImpactSpawnOffset;

	if (ProjectileVFXComponent && IsValid(InMainProjectileVFX))
	{
		ProjectileVFXComponent->SetAsset(InMainProjectileVFX);
	}

	if (ProjectileVFXMovementComp)
	{
		ProjectileVFXMovementComp->Velocity = InitialVelocity;
	}
}

void AGP_ProjectileMaster::OnProjectileCollisionBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!IsValid(OtherActor) || OtherActor == this)
	{
		return;
	}

	if (!OtherActor->IsA<ABlockingVolume>())
	{
		return;
	}

	if (ProjectileVFXMovementComp)
	{
		ProjectileVFXMovementComp->StopMovementImmediately();
	}

	if (ProjectileVFXComponent)
	{
		ProjectileVFXComponent->SetVariableBool(FName(TEXT("User.KillMainSlash")), true);
	}

	SpawnVFX(HitImpactVFX, MuzzleHitImpactSpawnOffset);

	if (ProjectileDestroyDelay <= 0.f)
	{
		DestroyAfterImpact();
		return;
	}

	FTimerHandle DestroyTimerHandle;
	GetWorldTimerManager().SetTimer(
		DestroyTimerHandle,
		this,
		&ThisClass::DestroyAfterImpact,
		ProjectileDestroyDelay,
		false);
}

void AGP_ProjectileMaster::DestroyAfterImpact()
{
	Destroy();
}

void AGP_ProjectileMaster::SpawnMuzzleVFX()
{
	SpawnVFX(MuzzleVFX, MuzzleHitImpactSpawnOffset);
}
