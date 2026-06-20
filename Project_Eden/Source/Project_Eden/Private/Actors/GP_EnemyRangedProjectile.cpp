#include "Actors/GP_EnemyRangedProjectile.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameplayEffect.h"
#include "GameplayTags/GP_Tags.h"
#include "Utils/GP_BlueprintLibrary.h"
#include "UObject/ConstructorHelpers.h"

AGP_EnemyRangedProjectile::AGP_EnemyRangedProjectile()
{
	SetReplicateMovement(true);

	ProjectileCollision = CreateDefaultSubobject<USphereComponent>(TEXT("ProjectileCollision"));
	ProjectileCollision->SetupAttachment(RootScene);
	ProjectileCollision->SetSphereRadius(24.0f);

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(ProjectileCollision);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileMesh->SetRelativeScale3D(FVector(0.24f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMeshFinder.Succeeded())
	{
		// The engine sphere is a visible prototype; designers can replace it in a Blueprint child later.
		ProjectileMesh->SetStaticMesh(SphereMeshFinder.Object);
	}

	if (ProjectileMovement)
	{
		// Move the native collision primitive rather than the non-primitive scene root inherited from AGP_Projectile.
		ProjectileMovement->SetUpdatedComponent(ProjectileCollision);
		ProjectileMovement->InitialSpeed = 1350.0f;
		ProjectileMovement->MaxSpeed = 1350.0f;
		ProjectileMovement->ProjectileGravityScale = 0.0f;
	}

	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageEffectFinder(TEXT("/Game/GAS_Pattern/AbilitySystem/GameplayEffects/Damage/GE_PrimaryDamage"));
	if (DamageEffectFinder.Succeeded())
	{
		DamageEffectClass = DamageEffectFinder.Class;
	}

	HitEventTag = GPTags::Event::Player::HitReact;
}

void AGP_EnemyRangedProjectile::LaunchToward(const FVector& WorldDirection)
{
	if (ProjectileMovement)
	{
		// Set velocity after spawn so actor rotation and inherited component defaults cannot redirect the shot.
		ProjectileMovement->Velocity = WorldDirection.GetSafeNormal() * ProjectileMovement->InitialSpeed;
	}
}

void AGP_EnemyRangedProjectile::OnProjectileOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || bHasHit || !UGP_BlueprintLibrary::CanApplyCombatEffect(GetInstigator(), OtherActor))
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetInstigator());
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
	if (!IsValid(SourceASC) || !IsValid(TargetASC) || !*DamageEffectClass)
	{
		return;
	}

	bHasHit = true;
	FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
	ContextHandle.AddInstigator(GetInstigator(), this);
	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, EffectLevel, ContextHandle);
	if (SpecHandle.IsValid())
	{
		// Enemy projectiles provide their own values because default enemy abilities are granted without a SkillData source object.
		SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Data::Base, BaseDamage);
		SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Data::BaseSpell, 0.0f);
		SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Data::ToughnessBase, 0.0f);
		SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Data::Multiplier, 1.0f);
		SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Coef::Atk, AttackPowerDamageCoefficient);
		SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Coef::M_Atk, 0.0f);
		SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Coef::Def, 0.0f);
		SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Coef::Hp, 0.0f);
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}

	TArray<AActor*> HitActors{OtherActor};
	UGP_BlueprintLibrary::SendGameplayEventToActors(GetInstigator(), HitActors, HitEventTag);
	MulticastPlayHitEffect(GetActorLocation(), GetActorRotation(), ImpactVisualActorClass, NAME_None, 1.0f);
	Destroy();
}
