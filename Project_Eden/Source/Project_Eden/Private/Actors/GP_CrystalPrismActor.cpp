#include "Actors/GP_CrystalPrismActor.h"

#include "Actors/GP_SeraphLaserActor.h"
#include "Characters/GP_CrystalSeraphBossCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AGP_CrystalPrismActor::AGP_CrystalPrismActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	ReflectionCollision = CreateDefaultSubobject<USphereComponent>(TEXT("ReflectionCollision"));
	ReflectionCollision->SetupAttachment(SceneRoot);
	ReflectionCollision->SetSphereRadius(CollisionRadius);
	ReflectionCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ReflectionCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	ReflectionCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	ReflectionCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	PrismMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrismMesh"));
	PrismMesh->SetupAttachment(SceneRoot);
	PrismMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PrismMesh->SetRelativeScale3D(FVector(1.6f, 1.6f, 2.2f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMeshFinder(TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (ConeMeshFinder.Succeeded())
	{
		PrismMesh->SetStaticMesh(ConeMeshFinder.Object);
	}
}

void AGP_CrystalPrismActor::BeginPlay()
{
	Super::BeginPlay();

	ReflectionCollision->SetSphereRadius(GetCollisionRadius());
	SetLifeSpan(FMath::Max(0.0f, PrismLifeSpan));
}

void AGP_CrystalPrismActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGP_CrystalPrismActor, BossOwner);
}

void AGP_CrystalPrismActor::InitializePrism(AGP_CrystalSeraphBossCharacter* InBossOwner)
{
	BossOwner = InBossOwner;
	SetOwner(InBossOwner);
	SetInstigator(InBossOwner);
}

bool AGP_CrystalPrismActor::NotifyLaserHit(AGP_SeraphLaserActor* LaserActor, const FVector& IncomingDirection)
{
	if (!HasAuthority() || !IsValid(BossOwner))
	{
		return false;
	}

	const FVector ReflectedDirection = ResolveReflectedDirection(IncomingDirection);
	if (ReflectedDirection.IsNearlyZero())
	{
		return false;
	}

	// Reflection success is the mechanic trigger; the wing core owns the actual damage window.
	BossOwner->RequestExposeWingCore();
	BP_OnLaserReflected(LaserActor, ReflectedDirection);
	return true;
}

FVector AGP_CrystalPrismActor::ResolveReflectedDirection(const FVector& IncomingDirection) const
{
	if (IsValid(BossOwner))
	{
		return (BossOwner->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	}

	const FVector SafeIncoming = IncomingDirection.GetSafeNormal();
	if (SafeIncoming.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	const FVector PrismNormal = GetActorForwardVector().GetSafeNormal();
	return FMath::GetReflectionVector(SafeIncoming, PrismNormal).GetSafeNormal();
}

float AGP_CrystalPrismActor::GetCollisionRadius() const
{
	return FMath::Max(0.0f, CollisionRadius);
}
