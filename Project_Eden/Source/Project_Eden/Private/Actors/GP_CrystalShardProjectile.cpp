#include "Actors/GP_CrystalShardProjectile.h"

#include "Actors/GP_CrystalSeraphCombatUtils.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameplayEffect.h"
#include "GameplayTags/GP_Tags.h"
#include "UObject/ConstructorHelpers.h"

AGP_CrystalShardProjectile::AGP_CrystalShardProjectile()
{
	ShardCollision = CreateDefaultSubobject<USphereComponent>(TEXT("ShardCollision"));
	ShardCollision->SetupAttachment(RootScene);
	ShardCollision->SetSphereRadius(36.0f);

	ShardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShardMesh"));
	ShardMesh->SetupAttachment(ShardCollision);
	ShardMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShardMesh->SetRelativeScale3D(FVector(0.35f, 0.35f, 0.8f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMeshFinder(TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (ConeMeshFinder.Succeeded())
	{
		ShardMesh->SetStaticMesh(ConeMeshFinder.Object);
	}

	if (ProjectileMovement)
	{
		ProjectileMovement->InitialSpeed = 1800.0f;
		ProjectileMovement->MaxSpeed = 1800.0f;
	}

	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageEffectFinder(TEXT("/Game/GAS_Pattern/AbilitySystem/GameplayEffects/Damage/GE_PrimaryDamage"));
	if (DamageEffectFinder.Succeeded())
	{
		DamageEffectClass = DamageEffectFinder.Class;
	}
}

void AGP_CrystalShardProjectile::OnProjectileOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || bHasHit || !IsValid(OtherActor) || OtherActor == GetInstigator() || OtherActor == this)
	{
		return;
	}

	bHasHit = true;
	TArray<AActor*> HitActors;
	HitActors.Add(OtherActor);

	GPCrystalSeraphCombatUtils::ApplySetByCallerDamage(
		GetInstigator(),
		HitActors,
		DamageEffectClass,
		GPTags::Event::Player::HitReact,
		AttackPowerDamageCoefficient,
		0.0f,
		0.0f,
		true);

	MulticastPlayHitEffect(GetActorLocation(), GetActorRotation(), ImpactVisualActorClass, NAME_None, 1.0f);
	Destroy();
}
