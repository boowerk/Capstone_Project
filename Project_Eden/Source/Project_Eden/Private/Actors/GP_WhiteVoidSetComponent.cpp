#include "Actors/GP_WhiteVoidSetComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

UGP_WhiteVoidSetComponent::UGP_WhiteVoidSetComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		FloorMesh = CubeMeshFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMeshFinder.Succeeded())
	{
		SkySphereMesh = SphereMeshFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WhiteMaterialFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (WhiteMaterialFinder.Succeeded())
	{
		WhiteMaterial = WhiteMaterialFinder.Object;
		SkyMaterial = WhiteMaterialFinder.Object;
	}
}

void UGP_WhiteVoidSetComponent::OnRegister()
{
	Super::OnRegister();
	RebuildWhiteVoidSet();
}

void UGP_WhiteVoidSetComponent::OnComponentCreated()
{
	Super::OnComponentCreated();
	RebuildWhiteVoidSet();
}

#if WITH_EDITOR
void UGP_WhiteVoidSetComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RebuildWhiteVoidSet();
}
#endif

void UGP_WhiteVoidSetComponent::RebuildWhiteVoidSet()
{
	EnsureChildComponents();

	if (FloorComponent)
	{
		FloorComponent->SetStaticMesh(FloorMesh);
		FloorComponent->SetMaterial(0, WhiteMaterial);
		FloorComponent->SetRelativeLocation(WhiteVoidOffset + FVector(0.0, 0.0, FloorCenterZOffset));
		FloorComponent->SetRelativeRotation(FRotator::ZeroRotator);
		FloorComponent->SetRelativeScale3D(FloorScale);
		FloorComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		FloorComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		FloorComponent->SetHiddenInGame(false);
		FloorComponent->SetVisibility(true);
	}

	if (SkySphereComponent)
	{
		SkySphereComponent->SetStaticMesh(SkySphereMesh);
		SkySphereComponent->SetMaterial(0, SkyMaterial ? SkyMaterial : WhiteMaterial);
		SkySphereComponent->SetRelativeLocation(WhiteVoidOffset);
		SkySphereComponent->SetRelativeRotation(FRotator::ZeroRotator);
		SkySphereComponent->SetRelativeScale3D(SkySphereScale);
		SkySphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SkySphereComponent->SetGenerateOverlapEvents(false);
		SkySphereComponent->SetCastShadow(false);
		SkySphereComponent->SetReverseCulling(true);
		SkySphereComponent->SetHiddenInGame(false);
		SkySphereComponent->SetVisibility(true);
	}

}

void UGP_WhiteVoidSetComponent::EnsureChildComponents()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	if (!FloorComponent)
	{
		FloorComponent = NewObject<UStaticMeshComponent>(Owner, TEXT("WhiteVoidFloor"));
		FloorComponent->SetupAttachment(this);
		FloorComponent->RegisterComponent();
	}

	if (!SkySphereComponent)
	{
		SkySphereComponent = NewObject<UStaticMeshComponent>(Owner, TEXT("WhiteVoidSkySphere"));
		SkySphereComponent->SetupAttachment(this);
		SkySphereComponent->RegisterComponent();
	}

}
