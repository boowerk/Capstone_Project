#include "Actors/GP_MultiHitProjectile.h"

#include "AbilitySystem/Abilities/GP_SkillData.h"
#include "Components/ShapeComponent.h"
#include "TimerManager.h"
#include "Utils/GP_BlueprintLibrary.h"

AGP_MultiHitProjectile::AGP_MultiHitProjectile()
{
	bDestroyOnHit = false;
}

void AGP_MultiHitProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (CollisionComponent)
	{
		CollisionComponent->SetNotifyRigidBodyCollision(true);
		CollisionComponent->OnComponentHit.AddDynamic(this, &ThisClass::OnMultiHitProjectileHit);
	}
}

void AGP_MultiHitProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopHitTimer();
	HitCandidates.Empty();

	Super::EndPlay(EndPlayReason);
}

void AGP_MultiHitProjectile::OnProjectileOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	const TWeakObjectPtr<AActor> TargetActor(OtherActor);
	if (!HasAuthority()
		|| !IsValid(OtherActor)
		|| OtherActor == GetInstigator()
		|| OtherActor == this
		|| HitCandidates.Contains(TargetActor)
		|| !UGP_BlueprintLibrary::CanApplyCombatEffect(GetInstigator(), OtherActor))
	{
		return;
	}

	HitCandidates.Add(TargetActor);

	if (TotalAppliedHits == 0)
	{
		ApplyNextHit();

		const int32 MaxHits = SkillData ? FMath::Max(1, SkillData->ProjectileMaxHitsPerTarget) : 3;
		if (TotalAppliedHits < MaxHits)
		{
			const float HitInterval = SkillData ? FMath::Max(0.01f, SkillData->ProjectileHitInterval) : 0.1f;
			GetWorldTimerManager().SetTimer(
				HitTimerHandle,
				this,
				&ThisClass::ApplyNextHit,
				HitInterval,
				true);
		}
	}
}

void AGP_MultiHitProjectile::OnMultiHitProjectileHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (!HasAuthority() || !IsValid(OtherActor) || OtherActor == GetInstigator())
	{
		return;
	}

	FVector ImpactLocation = GetActorLocation();
	if (!Hit.ImpactPoint.IsNearlyZero())
	{
		ImpactLocation = FVector(Hit.ImpactPoint);
	}

	MulticastPlayHitEffect(
		ImpactLocation,
		GetActorRotation(),
		ImpactVisualActorClass,
		NAME_None,
		1.0f);
	Destroy();
}

void AGP_MultiHitProjectile::ApplyNextHit()
{
	AActor* InstigatorActor = GetInstigator();
	if (!HasAuthority() || !IsValid(InstigatorActor))
	{
		StopHitTimer();
		return;
	}

	HitCandidates.RemoveAll([](const TWeakObjectPtr<AActor>& Candidate)
	{
		return !Candidate.IsValid();
	});

	if (HitCandidates.IsEmpty())
	{
		return;
	}

	const int32 MaxHits = SkillData ? FMath::Max(1, SkillData->ProjectileMaxHitsPerTarget) : 3;
	if (TotalAppliedHits >= MaxHits)
	{
		StopHitTimer();
		return;
	}

	NextCandidateIndex %= HitCandidates.Num();
	AActor* Target = HitCandidates[NextCandidateIndex].Get();
	NextCandidateIndex = (NextCandidateIndex + 1) % HitCandidates.Num();

	const float DamageScale = SkillData ? FMath::Max(0.0f, SkillData->ProjectileDamagePerHitMultiplier) : 0.4f;

	TArray<AActor*> HitActors;
	HitActors.Add(Target);
	UGP_BlueprintLibrary::ApplyGameplayEffectAndEventToActors(
		InstigatorActor,
		HitActors,
		DamageEffectClass,
		HitEventTag,
		EffectLevel,
		SkillData,
		DamageScale);

	++TotalAppliedHits;
	if (TotalAppliedHits >= MaxHits)
	{
		MulticastPlayHitEffect(
			Target->GetActorLocation(),
			GetActorRotation(),
			ImpactVisualActorClass,
			NAME_None,
			1.0f);
		StopHitTimer();
	}
}

void AGP_MultiHitProjectile::StopHitTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HitTimerHandle);
	}
}
