#include "Actors/GP_AreaProjectile.h"

#include "AbilitySystem/Abilities/GP_SkillData.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameplayEffect.h"
#include "GameplayTags/GP_Tags.h"
#include "Net/UnrealNetwork.h"
#include "Utils/GP_BlueprintLibrary.h"

AGP_AreaProjectile::AGP_AreaProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	SetRootComponent(CollisionComponent);

	CollisionComponent->SetSphereRadius(20.f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	CollisionComponent->SetNotifyRigidBodyCollision(true);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComponent;
	ProjectileMovement->InitialSpeed = 1200.f;
	ProjectileMovement->MaxSpeed = 1200.f;
	ProjectileMovement->ProjectileGravityScale = 1.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;

	HitEventTag = GPTags::Event::Enemy::HitReact;
}

void AGP_AreaProjectile::BeginPlay()
{
	Super::BeginPlay();

	SetLifeSpan(ProjectileLifeSpan);
	if (ProjectileVisualSystem)
	{
		BP_OnProjectileVisualSystemChanged(ProjectileVisualSystem);
	}

	if (CollisionComponent)
	{
		CollisionComponent->OnComponentHit.AddDynamic(this, &ThisClass::OnProjectileHit);
		CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnProjectileOverlap);

		if (GetInstigator())
		{
			CollisionComponent->IgnoreActorWhenMoving(GetInstigator(), true);
		}
	}
}

void AGP_AreaProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGP_AreaProjectile, ProjectileVisualSystem);
}

void AGP_AreaProjectile::SetProjectileVisualSystem(UNiagaraSystem* InProjectileVisualSystem)
{
	if (!InProjectileVisualSystem)
	{
		return;
	}

	ProjectileVisualSystem = InProjectileVisualSystem;
	BP_OnProjectileVisualSystemChanged(ProjectileVisualSystem);
}

void AGP_AreaProjectile::ApplyExplosionRadiusMultiplier(float RadiusMultiplier)
{
	const float SafeMultiplier = FMath::Max(RadiusMultiplier, 0.0f);
	ExplosionRadius *= SafeMultiplier;
	VisualScaleMultiplier *= SafeMultiplier;
}

void AGP_AreaProjectile::OnRep_ProjectileVisualSystem()
{
	if (ProjectileVisualSystem)
	{
		BP_OnProjectileVisualSystemChanged(ProjectileVisualSystem);
	}
}

void AGP_AreaProjectile::OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HasAuthority())
	{
		return;
	}

	Explode(Hit.ImpactPoint);
}

void AGP_AreaProjectile::OnProjectileOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!IsValid(OtherActor) || OtherActor == GetInstigator() || OtherActor == this)
	{
		return;
	}

	const FVector ImpactLocation = bFromSweep ? FVector(SweepResult.ImpactPoint) : GetActorLocation();
	Explode(ImpactLocation);
}

void AGP_AreaProjectile::Explode(const FVector& ImpactLocation)
{
	if (bHasExploded)
	{
		return;
	}

	bHasExploded = true;

	MulticastSpawnImpactVisual(ImpactLocation, ImpactVisualActorClass, VisualScaleMultiplier);

	if (AActor* InstigatorActor = GetInstigator())
	{
		const TArray<AActor*> HitActors = UGP_BlueprintLibrary::SphereOverlapActorsAtLocation(
			InstigatorActor,
			ImpactLocation,
			ExplosionRadius,
			InstigatorActor,
			bDrawDebug
		);

		UGP_BlueprintLibrary::ApplyGameplayEffectAndEventToActors(InstigatorActor, HitActors, DamageEffectClass, HitEventTag, EffectLevel, SkillData);
	}

	Destroy();
}

void AGP_AreaProjectile::MulticastSpawnImpactVisual_Implementation(const FVector& ImpactLocation, TSubclassOf<AActor> VisualActorClass, float VisualScale)
{
	if (!VisualActorClass)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* VisualActor = GetWorld()->SpawnActor<AActor>(
		VisualActorClass,
		ImpactLocation,
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (IsValid(VisualActor))
	{
		VisualActor->SetActorScale3D(FVector(FMath::Max(VisualScale, 0.0f)));
	}
}
