#include "Actors/GP_FabFenceActor.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "UObject/ConstructorHelpers.h"

AGP_FabFenceActor::AGP_FabFenceActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SceneRoot->SetMobility(EComponentMobility::Static);
	SetRootComponent(SceneRoot);

	NewelPostISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("NewelPostISM"));
	NewelPostISM->SetupAttachment(SceneRoot);

	BalusterISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("BalusterISM"));
	BalusterISM->SetupAttachment(SceneRoot);

	HandrailISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("HandrailISM"));
	HandrailISM->SetupAttachment(SceneRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> NewelPostMesh(TEXT("/Game/Fab/Model/NewelPost.NewelPost"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BalusterMesh(TEXT("/Game/Fab/Model/Baluster.Baluster"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> HandrailMesh(TEXT("/Game/Fab/Model/Handrail.Handrail"));

	if (NewelPostMesh.Succeeded())
	{
		NewelPostISM->SetStaticMesh(NewelPostMesh.Object);
	}
	if (BalusterMesh.Succeeded())
	{
		BalusterISM->SetStaticMesh(BalusterMesh.Object);
	}
	if (HandrailMesh.Succeeded())
	{
		HandrailISM->SetStaticMesh(HandrailMesh.Object);
	}
}

void AGP_FabFenceActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	RebuildFence();
}

void AGP_FabFenceActor::RebuildFence()
{
	if (!NewelPostISM || !BalusterISM || !HandrailISM)
	{
		return;
	}

	if (bUseScaleYAsLength)
	{
		const FVector ActorScale = GetActorScale3D();
		if (!FMath::IsNearlyEqual(ActorScale.Y, 1.0f))
		{
			Length = FMath::Max(0.0f, Length * ActorScale.Y);
			SetActorScale3D(FVector(ActorScale.X, 1.0f, ActorScale.Z));
		}
	}

	NewelPostISM->ClearInstances();
	BalusterISM->ClearInstances();
	HandrailISM->ClearInstances();

	const float SafeLength = FMath::Max(0.0f, Length);

	NewelPostISM->AddInstance(FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector::OneVector));
	if (SafeLength > KINDA_SMALL_NUMBER)
	{
		NewelPostISM->AddInstance(FTransform(FRotator::ZeroRotator, FVector(0.0f, SafeLength, 0.0f), FVector::OneVector));
	}

	AddInstancesAlongY(BalusterISM, BalusterStartOffset, SafeLength - BalusterEndOffset, BalusterSpacing);
	AddInstancesAlongY(HandrailISM, HandrailStartOffset, SafeLength + HandrailEndOverhang, HandrailSpacing);
}

void AGP_FabFenceActor::AddInstancesAlongY(UInstancedStaticMeshComponent* Component, float StartY, float EndY, float Spacing) const
{
	if (!Component || Spacing <= KINDA_SMALL_NUMBER || EndY < StartY)
	{
		return;
	}

	const int32 InstanceCount = FMath::FloorToInt((EndY - StartY) / Spacing) + 1;
	for (int32 Index = 0; Index < InstanceCount; ++Index)
	{
		const float Y = StartY + static_cast<float>(Index) * Spacing;
		Component->AddInstance(FTransform(FRotator::ZeroRotator, FVector(0.0f, Y, 0.0f), FVector::OneVector));
	}
}
