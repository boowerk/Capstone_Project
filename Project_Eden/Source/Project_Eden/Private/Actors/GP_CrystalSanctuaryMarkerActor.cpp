#include "Actors/GP_CrystalSanctuaryMarkerActor.h"

#include "Actors/GP_CrystalSeraphCombatUtils.h"
#include "Actors/GP_CrystalSeraphVFXDefaults.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffect.h"
#include "GameplayTags/GP_Tags.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "VFX/GP_VisualCueComponent.h"

AGP_CrystalSanctuaryMarkerActor::AGP_CrystalSanctuaryMarkerActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	VisualCueComponent = CreateDefaultSubobject<UGP_VisualCueComponent>(TEXT("VisualCueComponent"));
	VisualCueComponent->SetNiagaraTintOverride(true, GPCrystalSeraphVFXDefaults::GetCrystalTintColor());

	DamageArea = CreateDefaultSubobject<USphereComponent>(TEXT("DamageArea"));
	DamageArea->SetupAttachment(SceneRoot);
	DamageArea->SetSphereRadius(MarkerRadius);
	DamageArea->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DamageArea->SetCollisionResponseToAllChannels(ECR_Ignore);
	DamageArea->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	MarkerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MarkerMesh"));
	MarkerMesh->SetupAttachment(SceneRoot);
	MarkerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageEffectFinder(TEXT("/Game/GAS_Pattern/AbilitySystem/GameplayEffects/Damage/GE_PrimaryDamage"));
	if (DamageEffectFinder.Succeeded())
	{
		DamageEffectClass = DamageEffectFinder.Class;
	}

}

void AGP_CrystalSanctuaryMarkerActor::BeginPlay()
{
	Super::BeginPlay();

	if (MarkerMesh->GetStaticMesh() == nullptr && IsValid(MarkerStaticMesh.Get()))
	{
		MarkerMesh->SetStaticMesh(MarkerStaticMesh);
	}
	MarkerMesh->SetRelativeScale3D(MarkerMeshScale);
	if (IsValid(TelegraphVFX.Get()))
	{
		VisualCueComponent->AddNiagaraCue(GPTags::GameplayCue::Ability::Telegraph_Magic, TelegraphVFX);
	}
	if (IsValid(ExplosionVFX.Get()))
	{
		VisualCueComponent->AddNiagaraCue(GPTags::GameplayCue::Ability::Impact_Magic, ExplosionVFX);
	}
	DamageArea->SetSphereRadius(FMath::Max(0.0f, MarkerRadius));
	VisualCueComponent->ActivatePersistentCue(GPTags::GameplayCue::Ability::Telegraph_Magic, MarkerMesh, FVector::ZeroVector, FRotator::ZeroRotator, TelegraphVFXScale);
	BP_OnTelegraphStarted();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(ExplosionTimerHandle, this, &ThisClass::Explode, FMath::Max(0.0f, TelegraphDuration), false);
	}
}

void AGP_CrystalSanctuaryMarkerActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ExplosionTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void AGP_CrystalSanctuaryMarkerActor::InitializeSanctuaryMarker(AActor* InBossOwner)
{
	BossOwner = InBossOwner;
	SetOwner(InBossOwner);
	SetInstigator(Cast<APawn>(InBossOwner));
}

void AGP_CrystalSanctuaryMarkerActor::Explode()
{
	VisualCueComponent->DeactivatePersistentCue(GPTags::GameplayCue::Ability::Telegraph_Magic);
	if (HasAuthority())
	{
		TArray<AActor*> OverlappingActors;
		DamageArea->GetOverlappingActors(OverlappingActors);
		GPCrystalSeraphCombatUtils::ApplySetByCallerDamage(
			GetInstigator(),
			OverlappingActors,
			DamageEffectClass,
			GPTags::Event::Player::HitReact,
			AttackPowerDamageCoefficient,
			0.0f,
			0.0f,
			true);
		MulticastPlayExplosionVFX(GetActorLocation());
	}

	BP_OnExploded();
	Destroy();
}

void AGP_CrystalSanctuaryMarkerActor::MulticastPlayExplosionVFX_Implementation(const FVector& ExplosionLocation)
{
	if (IsValid(VisualCueComponent))
	{
		VisualCueComponent->PlayOneShotAtLocation(GPTags::GameplayCue::Ability::Impact_Magic, ExplosionLocation, FRotator::ZeroRotator, ExplosionVFXScale);
	}
}
