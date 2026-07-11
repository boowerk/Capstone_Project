#include "Game/Testing/GP_LandscapeTestFloorActor.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AGP_LandscapeTestFloorActor::AGP_LandscapeTestFloorActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	NavigationFloor = CreateDefaultSubobject<UBoxComponent>(TEXT("NavigationFloor"));
	SetRootComponent(NavigationFloor);
	NavigationFloor->SetMobility(EComponentMobility::Static);
	NavigationFloor->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	NavigationFloor->SetGenerateOverlapEvents(false);
	NavigationFloor->SetCanEverAffectNavigation(true);

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(NavigationFloor);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetGenerateOverlapEvents(false);
	VisualMesh->SetCanEverAffectNavigation(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded())
	{
		VisualMesh->SetStaticMesh(CubeFinder.Object);
	}

	RefreshFloorGeometry();
}

void AGP_LandscapeTestFloorActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshFloorGeometry();
}

void AGP_LandscapeTestFloorActor::ConfigureFloor(const FVector2D& InSize, float InThickness)
{
	FloorSize.X = FMath::Max(100.0f, InSize.X);
	FloorSize.Y = FMath::Max(100.0f, InSize.Y);
	FloorThickness = FMath::Max(10.0f, InThickness);
	RefreshFloorGeometry();
}

void AGP_LandscapeTestFloorActor::RefreshFloorGeometry()
{
	const FVector SafeExtent(
		FMath::Max(50.0f, FloorSize.X * 0.5f),
		FMath::Max(50.0f, FloorSize.Y * 0.5f),
		FMath::Max(5.0f, FloorThickness * 0.5f));
	if (NavigationFloor)
	{
		NavigationFloor->SetBoxExtent(SafeExtent);
	}
	if (VisualMesh)
	{
		// Engine cube dimensions are 100cm, matching the collision box without scaling the owning actor.
		VisualMesh->SetRelativeScale3D(FVector(
			SafeExtent.X / 50.0f,
			SafeExtent.Y / 50.0f,
			SafeExtent.Z / 50.0f));
	}
}
