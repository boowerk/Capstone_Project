#include "Actors/GP_CrystalPrismActor.h"

#include "Actors/GP_SeraphLaserActor.h"
#include "Characters/GP_CrystalSeraphBossCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameplayTags/GP_Tags.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"
#include "VFX/GP_VisualCueComponent.h"

AGP_CrystalPrismActor::AGP_CrystalPrismActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	VisualCueComponent = CreateDefaultSubobject<UGP_VisualCueComponent>(TEXT("VisualCueComponent"));

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
	PrismMesh->SetRelativeScale3D(PrismVisualScale);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMeshFinder(TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (ConeMeshFinder.Succeeded())
	{
		PrismMesh->SetStaticMesh(ConeMeshFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> AuraVFXFinder(TEXT("/Game/Imported_VFX/Free_Magic/VFX_Niagara/NS_Free_Magic_Aura.NS_Free_Magic_Aura"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> ReflectVFXFinder(TEXT("/Game/Imported_VFX/Free_Magic/VFX_Niagara/NS_Free_Magic_Hit2.NS_Free_Magic_Hit2"));
	if (AuraVFXFinder.Succeeded())
	{
		VisualCueComponent->AddNiagaraCue(GPTags::GameplayCue::Ability::Active_Magic, AuraVFXFinder.Object);
	}
	if (ReflectVFXFinder.Succeeded())
	{
		VisualCueComponent->AddNiagaraCue(GPTags::GameplayCue::Ability::Reflect_Magic, ReflectVFXFinder.Object);
	}
}

void AGP_CrystalPrismActor::BeginPlay()
{
	Super::BeginPlay();

	// Apply the editable scale at runtime so native and Blueprint prism classes use the same collision-readable size.
	PrismMesh->SetRelativeScale3D(FVector(
		FMath::Max(0.01f, PrismVisualScale.X),
		FMath::Max(0.01f, PrismVisualScale.Y),
		FMath::Max(0.01f, PrismVisualScale.Z)));
	ReflectionCollision->SetSphereRadius(GetCollisionRadius());
	const FVector SafeAuraScale(
		FMath::Max(0.01f, PrismAuraScale.X),
		FMath::Max(0.01f, PrismAuraScale.Y),
		FMath::Max(0.01f, PrismAuraScale.Z));
	VisualCueComponent->ActivatePersistentCue(GPTags::GameplayCue::Ability::Active_Magic, PrismMesh, FVector::ZeroVector, FRotator::ZeroRotator, SafeAuraScale);
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

	// Every successful reflected laser breaks one wing-core stage; the third stage drops the boss into groggy.
	BossOwner->RequestWingCoreBreak();
	BP_OnLaserReflected(LaserActor, ReflectedDirection);
	MulticastPlayReflectionVFX(GetActorLocation(), ReflectedDirection.Rotation());
	return true;
}

void AGP_CrystalPrismActor::MulticastPlayReflectionVFX_Implementation(const FVector& ReflectionLocation, const FRotator& ReflectionRotation)
{
	if (IsValid(VisualCueComponent))
	{
		VisualCueComponent->PlayOneShotAtLocation(GPTags::GameplayCue::Ability::Reflect_Magic, ReflectionLocation, ReflectionRotation, FVector(1.35f));
	}
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
