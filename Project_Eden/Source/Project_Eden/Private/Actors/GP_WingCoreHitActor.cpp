#include "Actors/GP_WingCoreHitActor.h"

#include "Characters/GP_CrystalSeraphBossCharacter.h"
#include "Characters/GP_CrystalSeraphStateComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AGP_WingCoreHitActor::AGP_WingCoreHitActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	CoreCollision = CreateDefaultSubobject<USphereComponent>(TEXT("CoreCollision"));
	CoreCollision->SetupAttachment(SceneRoot);
	CoreCollision->SetSphereRadius(CoreRadius);
	CoreCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CoreCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	CoreCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CoreCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);

	CoreMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoreMesh"));
	CoreMesh->SetupAttachment(SceneRoot);
	CoreMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CoreMesh->SetRelativeScale3D(FVector(1.6f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMeshFinder.Succeeded())
	{
		CoreMesh->SetStaticMesh(SphereMeshFinder.Object);
	}
}

void AGP_WingCoreHitActor::BeginPlay()
{
	Super::BeginPlay();

	CoreCollision->SetSphereRadius(FMath::Max(0.0f, CoreRadius));
	SetCoreActive(false);
}

void AGP_WingCoreHitActor::InitializeWingCore(AGP_CrystalSeraphBossCharacter* InBossOwner, UGP_CrystalSeraphStateComponent* InStateComponent)
{
	BossOwner = InBossOwner;
	StateComponent = InStateComponent;
	SetOwner(InBossOwner);
	SetInstigator(InBossOwner);
}

void AGP_WingCoreHitActor::SetCoreActive(bool bNewActive)
{
	SetActorHiddenInGame(!bNewActive);
	SetActorEnableCollision(bNewActive);
	CoreCollision->SetCollisionEnabled(bNewActive ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	BP_OnCoreActiveChanged(bNewActive);
}
