#include "Actors/GP_CrystalShardProjectile.h"

#include "Actors/GP_CrystalSeraphCombatUtils.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameplayEffect.h"
#include "GameplayTags/GP_Tags.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"
#include "VFX/GP_VisualCueComponent.h"

AGP_CrystalShardProjectile::AGP_CrystalShardProjectile()
{
	VisualCueComponent = CreateDefaultSubobject<UGP_VisualCueComponent>(TEXT("VisualCueComponent"));

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

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> ActiveVFXFinder(TEXT("/Game/Imported_VFX/Free_Magic/VFX_Niagara/NS_Free_Magic_Projectile2.NS_Free_Magic_Projectile2"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> ImpactVFXFinder(TEXT("/Game/Imported_VFX/Free_Magic/VFX_Niagara/NS_Free_Magic_Hit1.NS_Free_Magic_Hit1"));
	if (ActiveVFXFinder.Succeeded())
	{
		VisualCueComponent->AddNiagaraCue(GPTags::GameplayCue::Ability::Active_Magic, ActiveVFXFinder.Object);
	}
	if (ImpactVFXFinder.Succeeded())
	{
		VisualCueComponent->AddNiagaraCue(GPTags::GameplayCue::Ability::Impact_Magic, ImpactVFXFinder.Object);
	}
}

void AGP_CrystalShardProjectile::BeginPlay()
{
	Super::BeginPlay();
	VisualCueComponent->ActivatePersistentCue(GPTags::GameplayCue::Ability::Active_Magic, ShardCollision, FVector::ZeroVector, FRotator::ZeroRotator, FVector(0.8f));
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
	MulticastPlayShardImpactVFX(GetActorLocation(), GetActorRotation());
	Destroy();
}

void AGP_CrystalShardProjectile::MulticastPlayShardImpactVFX_Implementation(const FVector& ImpactLocation, const FRotator& ImpactRotation)
{
	if (IsValid(VisualCueComponent))
	{
		VisualCueComponent->PlayOneShotAtLocation(GPTags::GameplayCue::Ability::Impact_Magic, ImpactLocation, ImpactRotation);
	}
}
