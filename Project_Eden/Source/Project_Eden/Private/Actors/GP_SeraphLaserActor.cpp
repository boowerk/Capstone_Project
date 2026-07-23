#include "Actors/GP_SeraphLaserActor.h"

#include "Actors/GP_CrystalPrismActor.h"
#include "Actors/GP_CrystalSeraphCombatUtils.h"
#include "Actors/GP_CrystalSeraphVFXDefaults.h"
#include "Characters/GP_CrystalSeraphBossCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "GameplayTags/GP_Tags.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "VFX/GP_VisualCueComponent.h"

AGP_SeraphLaserActor::AGP_SeraphLaserActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	VisualCueComponent = CreateDefaultSubobject<UGP_VisualCueComponent>(TEXT("VisualCueComponent"));
	VisualCueComponent->SetNiagaraTintOverride(true, GPCrystalSeraphVFXDefaults::GetCrystalTintColor());
	ReflectionBeamVFXComponent = CreateDefaultSubobject<UGP_VisualCueComponent>(TEXT("ReflectionBeamVFXComponent"));
	ReflectionBeamVFXComponent->SetNiagaraTintOverride(true, GPCrystalSeraphVFXDefaults::GetCrystalTintColor());

	DamageBox = CreateDefaultSubobject<UBoxComponent>(TEXT("DamageBox"));
	DamageBox->SetupAttachment(SceneRoot);
	DamageBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DamageBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	DamageBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	LaserMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LaserMesh"));
	LaserMesh->SetupAttachment(DamageBox);
	LaserMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageEffectFinder(TEXT("/Game/GAS_Pattern/AbilitySystem/GameplayEffects/Damage/GE_PrimaryDamage"));
	if (DamageEffectFinder.Succeeded())
	{
		DamageEffectClass = DamageEffectFinder.Class;
	}

}

void AGP_SeraphLaserActor::BeginPlay()
{
	Super::BeginPlay();

	ConfigureVisualCues();
	UpdateLaserShape();
	VisualCueComponent->ActivatePersistentCue(GPTags::GameplayCue::Ability::Telegraph_Magic, DamageBox);
	BP_OnLaserTelegraphStarted();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(ActivationTimerHandle, this, &ThisClass::ActivateLaser, FMath::Max(0.0f, TelegraphDuration), false);
		World->GetTimerManager().SetTimer(EndTimerHandle, this, &ThisClass::FinishLaser, FMath::Max(0.0f, TelegraphDuration + ActiveDuration), false);
	}
}

void AGP_SeraphLaserActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ActivationTimerHandle);
		World->GetTimerManager().ClearTimer(DamageTimerHandle);
		World->GetTimerManager().ClearTimer(EndTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void AGP_SeraphLaserActor::InitializeLaser(AGP_CrystalSeraphBossCharacter* InBossOwner, AActor* InTargetActor, const FVector& InDirection, int32 InRemainingReflections)
{
	BossOwner = InBossOwner;
	TargetActor = InTargetActor;
	SetOwner(InBossOwner);
	SetInstigator(InBossOwner);

	LaserDirection = InDirection.GetSafeNormal();
	if (LaserDirection.IsNearlyZero())
	{
		LaserDirection = GetActorForwardVector().IsNearlyZero() ? FVector::ForwardVector : GetActorForwardVector();
	}

	RemainingReflections = FMath::Max(0, InRemainingReflections);
	SetActorRotation(LaserDirection.Rotation());
	UpdateLaserShape();
}

void AGP_SeraphLaserActor::ActivateLaser()
{
	bLaserActive = true;
	VisualCueComponent->DeactivatePersistentCue(GPTags::GameplayCue::Ability::Telegraph_Magic);
	// DamageBox is centered along the beam for collision, while the active VFX must begin at the firing origin.
	VisualCueComponent->ActivatePersistentCue(GPTags::GameplayCue::Ability::Active_Magic, SceneRoot);
	DamageBox->SetCollisionEnabled(bCanDealDamage ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	BP_OnLaserActivated();
	TryReflectFromPrism();
	ApplyLaserDamageTick();

	if (bCanDealDamage)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(DamageTimerHandle, this, &ThisClass::ApplyLaserDamageTick, FMath::Max(0.01f, DamageTickInterval), true);
		}
	}
}

void AGP_SeraphLaserActor::FinishLaser()
{
	if (HasAuthority() && bReleasesPrismShields && IsValid(BossOwner))
	{
		BossOwner->ReleaseCrystalPrismShields();
	}
	Destroy();
}

void AGP_SeraphLaserActor::ApplyLaserDamageTick()
{
	if (!HasAuthority() || !bLaserActive || !bCanDealDamage)
	{
		return;
	}

	TArray<AActor*> OverlappingActors;
	DamageBox->GetOverlappingActors(OverlappingActors);
	GPCrystalSeraphCombatUtils::ApplySetByCallerDamage(
		GetInstigator(),
		OverlappingActors,
		DamageEffectClass,
		GPTags::Event::Player::HitReact,
		0.0f,
		MaxHealthDamagePerTick,
		0.0f,
		true);
}

bool AGP_SeraphLaserActor::TryReflectFromPrism()
{
	if (!HasAuthority() || bHasReflected || RemainingReflections <= 0 || !GetWorld())
	{
		return false;
	}

	TArray<AActor*> PrismActors;
	UGameplayStatics::GetAllActorsOfClass(this, AGP_CrystalPrismActor::StaticClass(), PrismActors);

	AGP_CrystalPrismActor* BestPrism = nullptr;
	float BestDistanceAlongLaser = TNumericLimits<float>::Max();
	FVector BestReflectionLocation = FVector::ZeroVector;
	const FVector LaserStart = GetActorLocation();
	const FVector SafeDirection = LaserDirection.GetSafeNormal();

	for (AActor* Actor : PrismActors)
	{
		AGP_CrystalPrismActor* PrismActor = Cast<AGP_CrystalPrismActor>(Actor);
		if (!IsValid(PrismActor))
		{
			continue;
		}

		const FVector PrismCenter = PrismActor->GetReflectionCenter();
		const FVector ToPrism = PrismCenter - LaserStart;
		const float DistanceAlongLaser = FVector::DotProduct(ToPrism, SafeDirection);
		if (DistanceAlongLaser < 0.0f || DistanceAlongLaser > LaserLength)
		{
			continue;
		}

		const FVector ClosestPoint = LaserStart + SafeDirection * DistanceAlongLaser;
		const float ShieldRadius = PrismActor->GetCollisionRadius();
		const float DistanceFromLine = FVector::Dist(ClosestPoint, PrismCenter);
		if (DistanceFromLine > ShieldRadius + LaserWidth * 0.5f)
		{
			continue;
		}

		FVector ReflectionLocation = FVector::ZeroVector;
		if (DistanceFromLine <= ShieldRadius)
		{
			const float HalfChordLength = FMath::Sqrt(FMath::Max(0.0f, FMath::Square(ShieldRadius) - FMath::Square(DistanceFromLine)));
			const float EntryDistance = DistanceAlongLaser - HalfChordLength;
			const float ExitDistance = DistanceAlongLaser + HalfChordLength;
			const float SurfaceDistance = EntryDistance >= 0.0f ? EntryDistance : ExitDistance;
			ReflectionLocation = LaserStart + SafeDirection * FMath::Clamp(SurfaceDistance, 0.0f, LaserLength);
		}
		else
		{
			// The beam width grazed the shield. Place feedback on the nearest point of the visible sphere.
			ReflectionLocation = PrismCenter + (ClosestPoint - PrismCenter).GetSafeNormal() * ShieldRadius;
		}

		if (DistanceAlongLaser < BestDistanceAlongLaser)
		{
			BestDistanceAlongLaser = DistanceAlongLaser;
			BestPrism = PrismActor;
			BestReflectionLocation = ReflectionLocation;
		}
	}

	if (!IsValid(BestPrism))
	{
		return false;
	}

	bHasReflected = BestPrism->NotifyLaserHitAtLocation(this, LaserDirection, BestReflectionLocation);
	if (!bHasReflected)
	{
		return false;
	}

	// The shield consumed the incoming beam. Keep its visuals for the pattern, but do not let the full
	// pre-reflection DamageBox or the new outbound beam continue hurting the player.
	bCanDealDamage = false;
	DamageBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DamageTimerHandle);
	}

	const FVector ReflectedDirection = BestPrism->ResolveReflectedDirection(LaserDirection);
	SpawnReflectedSegment(BestReflectionLocation, ReflectedDirection);
	MulticastPlayReflectedVFX(BestReflectionLocation, ReflectedDirection.Rotation());
	BP_OnLaserReflected(BestReflectionLocation, ReflectedDirection);
	return true;
}

void AGP_SeraphLaserActor::MulticastPlayReflectedVFX_Implementation(const FVector& ReflectionOrigin, const FRotator& ReflectionRotation)
{
	if (IsValid(VisualCueComponent))
	{
		VisualCueComponent->PlayOneShotAtLocation(GPTags::GameplayCue::Ability::Reflect_Magic, ReflectionOrigin, ReflectionRotation, FVector(1.35f));
	}
	if (IsValid(ReflectionBeamVFXComponent))
	{
		// Keep crystal impact burst, then layer beam/lightning energy over each reflected segment origin.
		ReflectionBeamVFXComponent->PlayOneShotAtLocation(GPTags::GameplayCue::Ability::Reflect_Magic, ReflectionOrigin, ReflectionRotation, FVector(1.1f));
	}
}

void AGP_SeraphLaserActor::ConfigureVisualCues()
{
	if (IsValid(TelegraphVFX.Get()))
	{
		VisualCueComponent->AddNiagaraCue(GPTags::GameplayCue::Ability::Telegraph_Magic, TelegraphVFX);
	}
	if (IsValid(ActiveVFX.Get()))
	{
		VisualCueComponent->AddNiagaraCue(GPTags::GameplayCue::Ability::Active_Magic, ActiveVFX);
	}
	if (IsValid(ReflectionImpactVFX.Get()))
	{
		VisualCueComponent->AddNiagaraCue(GPTags::GameplayCue::Ability::Reflect_Magic, ReflectionImpactVFX);
	}
	if (IsValid(ReflectionBeamVFX.Get()))
	{
		ReflectionBeamVFXComponent->AddNiagaraCue(GPTags::GameplayCue::Ability::Reflect_Magic, ReflectionBeamVFX);
	}
}

void AGP_SeraphLaserActor::SpawnReflectedSegment(const FVector& ReflectionOrigin, const FVector& ReflectedDirection)
{
	if (!HasAuthority() || RemainingReflections <= 0 || ReflectedDirection.IsNearlyZero() || !GetWorld())
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = GetOwner();
	SpawnParameters.Instigator = GetInstigator();
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AGP_SeraphLaserActor* ReflectedLaser = GetWorld()->SpawnActor<AGP_SeraphLaserActor>(
		GetClass(),
		ReflectionOrigin + ReflectedDirection.GetSafeNormal() * 40.0f,
		ReflectedDirection.Rotation(),
		SpawnParameters);

	if (IsValid(ReflectedLaser))
	{
		ReflectedLaser->TelegraphDuration = 0.0f;
		ReflectedLaser->ActiveDuration = ActiveDuration;
		ReflectedLaser->LaserLength = LaserLength * 0.65f;
		ReflectedLaser->LaserWidth = LaserWidth;
		ReflectedLaser->DamageTickInterval = DamageTickInterval;
		ReflectedLaser->MaxHealthDamagePerTick = MaxHealthDamagePerTick;
		ReflectedLaser->DamageEffectClass = DamageEffectClass;
		ReflectedLaser->bCanDealDamage = false;
		ReflectedLaser->bReleasesPrismShields = false;
		ReflectedLaser->InitializeLaser(BossOwner, TargetActor.Get(), ReflectedDirection, RemainingReflections - 1);
	}
}

void AGP_SeraphLaserActor::UpdateLaserShape()
{
	const float HalfLength = FMath::Max(0.0f, LaserLength) * 0.5f;
	const float HalfWidth = FMath::Max(1.0f, LaserWidth) * 0.5f;
	DamageBox->SetBoxExtent(FVector(HalfLength, HalfWidth, HalfWidth));
	DamageBox->SetRelativeLocation(FVector(HalfLength, 0.0f, 0.0f));

	// The cube mesh is scaled to match the damage box, giving designers an immediate prototype visual.
	LaserMesh->SetRelativeScale3D(FVector(FMath::Max(0.01f, LaserLength / 100.0f), FMath::Max(0.01f, LaserWidth / 100.0f), FMath::Max(0.01f, LaserWidth / 100.0f)));
}
