#include "Actors/GP_CrystalShardProjectile.h"

#include "Actors/GP_CrystalSeraphCombatUtils.h"
#include "Actors/GP_CrystalSeraphVFXDefaults.h"
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
	VisualCueComponent->SetNiagaraTintOverride(true, GPCrystalSeraphVFXDefaults::GetCrystalTintColor());

	ShardCollision = CreateDefaultSubobject<USphereComponent>(TEXT("ShardCollision"));
	ShardCollision->SetupAttachment(RootScene);
	ShardCollision->SetSphereRadius(36.0f);

	ShardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShardMesh"));
	ShardMesh->SetupAttachment(ShardCollision);
	ShardMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

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

void AGP_CrystalShardProjectile::BeginPlay()
{
	Super::BeginPlay();
	if (ShardMesh->GetStaticMesh() == nullptr && IsValid(ShardStaticMesh.Get()))
	{
		ShardMesh->SetStaticMesh(ShardStaticMesh);
	}
	ShardMesh->SetRelativeScale3D(ShardMeshScale);
	if (IsValid(ProjectileActiveVFX.Get()))
	{
		VisualCueComponent->AddNiagaraCue(GPTags::GameplayCue::Ability::Active_Magic, ProjectileActiveVFX);
	}
	if (IsValid(ProjectileImpactVFX.Get()))
	{
		VisualCueComponent->AddNiagaraCue(GPTags::GameplayCue::Ability::Impact_Magic, ProjectileImpactVFX);
	}
	VisualCueComponent->ActivatePersistentCue(GPTags::GameplayCue::Ability::Active_Magic, ShardCollision, FVector::ZeroVector, FRotator::ZeroRotator, ProjectileActiveVFXScale);
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
		VisualCueComponent->PlayOneShotAtLocation(GPTags::GameplayCue::Ability::Impact_Magic, ImpactLocation, ImpactRotation, ProjectileImpactVFXScale);
	}
}
