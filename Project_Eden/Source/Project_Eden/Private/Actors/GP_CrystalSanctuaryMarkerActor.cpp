#include "Actors/GP_CrystalSanctuaryMarkerActor.h"

#include "Actors/GP_CrystalSeraphCombatUtils.h"
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

	DamageArea = CreateDefaultSubobject<USphereComponent>(TEXT("DamageArea"));
	DamageArea->SetupAttachment(SceneRoot);
	DamageArea->SetSphereRadius(MarkerRadius);
	DamageArea->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DamageArea->SetCollisionResponseToAllChannels(ECR_Ignore);
	DamageArea->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	MarkerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MarkerMesh"));
	MarkerMesh->SetupAttachment(SceneRoot);
	MarkerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MarkerMesh->SetRelativeScale3D(FVector(4.4f, 4.4f, 0.05f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMeshFinder.Succeeded())
	{
		MarkerMesh->SetStaticMesh(CylinderMeshFinder.Object);
	}

	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageEffectFinder(TEXT("/Game/GAS_Pattern/AbilitySystem/GameplayEffects/Damage/GE_PrimaryDamage"));
	if (DamageEffectFinder.Succeeded())
	{
		DamageEffectClass = DamageEffectFinder.Class;
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> TelegraphVFXFinder(TEXT("/Game/Imported_VFX/Free_Magic/VFX_Niagara/NS_Free_Magic_Circle2.NS_Free_Magic_Circle2"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> ImpactVFXFinder(TEXT("/Game/Imported_VFX/Free_Magic/VFX_Niagara/NS_Free_Magic_Area2.NS_Free_Magic_Area2"));
	if (TelegraphVFXFinder.Succeeded())
	{
		VisualCueComponent->AddNiagaraCue(GPTags::GameplayCue::Ability::Telegraph_Magic, TelegraphVFXFinder.Object);
	}
	if (ImpactVFXFinder.Succeeded())
	{
		VisualCueComponent->AddNiagaraCue(GPTags::GameplayCue::Ability::Impact_Magic, ImpactVFXFinder.Object);
	}
}

void AGP_CrystalSanctuaryMarkerActor::BeginPlay()
{
	Super::BeginPlay();

	DamageArea->SetSphereRadius(FMath::Max(0.0f, MarkerRadius));
	VisualCueComponent->ActivatePersistentCue(GPTags::GameplayCue::Ability::Telegraph_Magic, MarkerMesh, FVector::ZeroVector, FRotator::ZeroRotator, FVector(2.2f));
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
		VisualCueComponent->PlayOneShotAtLocation(GPTags::GameplayCue::Ability::Impact_Magic, ExplosionLocation, FRotator::ZeroRotator, FVector(2.0f));
	}
}
