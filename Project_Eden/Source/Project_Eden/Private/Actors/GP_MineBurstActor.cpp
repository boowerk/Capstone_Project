#include "Actors/GP_MineBurstActor.h"

#include "AbilitySystem/Abilities/GP_SkillData.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffect.h"
#include "GameplayTags/GP_Tags.h"
#include "TimerManager.h"
#include "Utils/GP_BlueprintLibrary.h"

AGP_MineBurstActor::AGP_MineBurstActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	TriggerComponent = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerComponent"));
	TriggerComponent->SetupAttachment(RootScene);
	TriggerComponent->SetSphereRadius(TriggerRadius);
	TriggerComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerComponent->SetGenerateOverlapEvents(true);

	HitEventTag = GPTags::Event::Enemy::HitReact;
}

void AGP_MineBurstActor::BeginPlay()
{
	Super::BeginPlay();

	SetLifeSpan(MineLifeSpan);

	if (TriggerComponent)
	{
		TriggerComponent->SetSphereRadius(TriggerRadius);
		TriggerComponent->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnTriggerOverlap);

		if (GetInstigator())
		{
			TriggerComponent->IgnoreActorWhenMoving(GetInstigator(), true);
		}
	}

	if (ArmDelay <= 0.f)
	{
		ArmMine();
		return;
	}

	GetWorldTimerManager().SetTimer(
		ArmTimerHandle,
		this,
		&ThisClass::ArmMine,
		ArmDelay,
		false
	);
}

void AGP_MineBurstActor::ApplyExplosionRadiusMultiplier(float RadiusMultiplier)
{
	const float SafeMultiplier = FMath::Max(RadiusMultiplier, 0.0f);
	ExplosionRadius *= SafeMultiplier;
	VisualScaleMultiplier *= SafeMultiplier;
}

void AGP_MineBurstActor::OnTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!bIsArmed || !IsValid(OtherActor) || OtherActor == GetInstigator() || OtherActor == this)
	{
		return;
	}

	Explode();
}

void AGP_MineBurstActor::ArmMine()
{
	bIsArmed = true;

	if (!HasAuthority() || !TriggerComponent)
	{
		return;
	}

	TArray<AActor*> OverlappingActors;
	TriggerComponent->GetOverlappingActors(OverlappingActors, APawn::StaticClass());

	for (AActor* OverlappingActor : OverlappingActors)
	{
		if (IsValid(OverlappingActor) && OverlappingActor != GetInstigator() && OverlappingActor != this)
		{
			Explode();
			return;
		}
	}
}

void AGP_MineBurstActor::Explode()
{
	if (bHasExploded)
	{
		return;
	}

	bHasExploded = true;
	const FVector ImpactLocation = GetActorLocation();

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

void AGP_MineBurstActor::MulticastSpawnImpactVisual_Implementation(const FVector& ImpactLocation, TSubclassOf<AActor> VisualActorClass, float VisualScale)
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
