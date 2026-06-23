#include "Actors/GP_DarkWaveProjectile.h"

#include "Actors/GP_BossCombatUtils.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameplayEffect.h"
#include "UObject/ConstructorHelpers.h"
#include "Utils/GP_BlueprintLibrary.h"

AGP_DarkWaveProjectile::AGP_DarkWaveProjectile()
{
	WaveCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("WaveCollision"));
	WaveCollision->SetupAttachment(RootScene);
	WaveCollision->SetBoxExtent(FVector(45.0f, 80.0f, 100.0f));

	WaveMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WaveMesh"));
	WaveMesh->SetupAttachment(WaveCollision);
	WaveMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WaveMesh->SetRelativeScale3D(FVector(0.12f, 1.6f, 2.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		// A thin cube is a readable temporary sword-wave silhouette and remains editable on child Blueprints.
		WaveMesh->SetStaticMesh(CubeMeshFinder.Object);
	}

	if (ProjectileMovement)
	{
		ProjectileMovement->InitialSpeed = 1700.0f;
		ProjectileMovement->MaxSpeed = 1700.0f;
	}
	ProjectileLifeSpan = 1.5f;

	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageEffectFinder(TEXT("/Game/GAS_Pattern/AbilitySystem/GameplayEffects/Damage/GE_PrimaryDamage"));
	if (DamageEffectFinder.Succeeded())
	{
		DamageEffectClass = DamageEffectFinder.Class;
	}
}

void AGP_DarkWaveProjectile::OnProjectileOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || bHasHit || !IsValid(OtherActor) || OtherActor == GetInstigator() || OtherActor == this
		|| !UGP_BlueprintLibrary::CanApplyCombatEffect(GetInstigator(), OtherActor))
	{
		return;
	}

	bHasHit = true;
	TArray<AActor*> Targets { OtherActor };
	GPBossCombatUtils::ApplyDamage(GetInstigator(), Targets, DamageEffectClass, AttackPowerDamageCoefficient, 0.0f, true, GPTags::Damage::Element::Chaos);
	MulticastPlayHitEffect(GetActorLocation(), GetActorRotation(), ImpactVisualActorClass, NAME_None, 1.0f);
	Destroy();
}
