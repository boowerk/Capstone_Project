#include "Actors/GP_DarkKnightGroundCrackActor.h"

#include "Actors/GP_BossCombatUtils.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffect.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AGP_DarkKnightGroundCrackActor::AGP_DarkKnightGroundCrackActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	DamageArea = CreateDefaultSubobject<USphereComponent>(TEXT("DamageArea"));
	DamageArea->SetupAttachment(SceneRoot);
	DamageArea->SetSphereRadius(CrackRadius);
	DamageArea->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DamageArea->SetCollisionResponseToAllChannels(ECR_Ignore);
	DamageArea->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	CrackMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CrackMesh"));
	CrackMesh->SetupAttachment(SceneRoot);
	CrackMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CrackMesh->SetRelativeScale3D(FVector(4.8f, 4.8f, 0.04f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMeshFinder.Succeeded())
	{
		// The flat cylinder is a replaceable placeholder for a crack decal/VFX Blueprint.
		CrackMesh->SetStaticMesh(CylinderMeshFinder.Object);
	}

	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageEffectFinder(TEXT("/Game/GAS_Pattern/AbilitySystem/GameplayEffects/Damage/GE_PrimaryDamage"));
	if (DamageEffectFinder.Succeeded())
	{
		DamageEffectClass = DamageEffectFinder.Class;
	}
}

void AGP_DarkKnightGroundCrackActor::BeginPlay()
{
	Super::BeginPlay();
	DamageArea->SetSphereRadius(FMath::Max(0.0f, CrackRadius));
	BP_OnTelegraphStarted();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(ExplosionTimerHandle, this, &ThisClass::Explode, FMath::Max(0.01f, TelegraphDuration), false);
	}
}

void AGP_DarkKnightGroundCrackActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ExplosionTimerHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void AGP_DarkKnightGroundCrackActor::InitializeGroundCrack(AActor* InBossOwner)
{
	SetOwner(InBossOwner);
	SetInstigator(Cast<APawn>(InBossOwner));
}

void AGP_DarkKnightGroundCrackActor::Explode()
{
	if (HasAuthority())
	{
		TArray<AActor*> Targets;
		DamageArea->GetOverlappingActors(Targets);
		GPBossCombatUtils::ApplyDamage(GetInstigator(), Targets, DamageEffectClass, AttackPowerDamageCoefficient, 0.0f, false, GPTags::Damage::Element::Brute);
	}
	BP_OnGroundCrackExploded();
	Destroy();
}
