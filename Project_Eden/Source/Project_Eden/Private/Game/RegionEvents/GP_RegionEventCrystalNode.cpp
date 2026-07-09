#include "Game/RegionEvents/GP_RegionEventCrystalNode.h"

#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AGP_RegionEventCrystalNode::AGP_RegionEventCrystalNode()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	CrystalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CrystalMesh"));
	SetRootComponent(CrystalMesh);
	CrystalMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CrystalMesh->SetCollisionObjectType(ECC_WorldDynamic);
	CrystalMesh->SetCollisionResponseToAllChannels(ECR_Block);
	CrystalMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CrystalMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	CrystalMesh->SetGenerateOverlapEvents(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CrystalMeshFinder(TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (CrystalMeshFinder.Succeeded())
	{
		CrystalMesh->SetStaticMesh(CrystalMeshFinder.Object);
	}
}

void AGP_RegionEventCrystalNode::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = FMath::Max(1.0f, MaxHealth);
	CrystalMesh->SetWorldScale3D(VisualScale);
}

void AGP_RegionEventCrystalNode::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGP_RegionEventCrystalNode, CurrentHealth);
}

void AGP_RegionEventCrystalNode::ApplyRegionEventHit(AActor* HitInstigator, float HitDamage)
{
	if (!HasAuthority() || bDestroyed)
	{
		return;
	}

	CurrentHealth = FMath::Max(0.0f, CurrentHealth - FMath::Max(0.0f, HitDamage));
	if (CurrentHealth <= UE_KINDA_SMALL_NUMBER)
	{
		DestroyCrystal(HitInstigator);
	}
}

void AGP_RegionEventCrystalNode::DestroyCrystal(AActor* DestroyingActor)
{
	if (bDestroyed)
	{
		return;
	}

	bDestroyed = true;
	OnCrystalNodeDestroyed.Broadcast(this, DestroyingActor);

	// Hide immediately on the server; replicated destroy removes it for clients after the event callback runs.
	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
	Destroy();
}
