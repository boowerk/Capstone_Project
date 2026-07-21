#include "Game/WorldLayout/GP_VillageSlot.h"

#include "Components/BoxComponent.h"

AGP_VillageSlot::AGP_VillageSlot()
{
	PrimaryActorTick.bCanEverTick = false;

	SlotBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("SlotBounds"));
	SetRootComponent(SlotBounds);
	SlotBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SlotBounds->SetGenerateOverlapEvents(false);
	SlotBounds->SetBoxExtent(FVector(5000.0f, 5000.0f, 1000.0f));
	SlotBounds->SetHiddenInGame(true);
	SlotBounds->ShapeColor = FColor(90, 90, 90);

#if WITH_EDITORONLY_DATA
	bIsSpatiallyLoaded = false;
#endif
}

FGP_VillageCandidate AGP_VillageSlot::MakeCandidate() const
{
	FGP_VillageCandidate Candidate;
	Candidate.SlotId = SlotId;
	Candidate.GroupId = GroupId;
	Candidate.SelectionWeight = SelectionWeight;
	return Candidate;
}

void AGP_VillageSlot::SetSelectedForRun(bool bSelected)
{
	if (bSelectedForRun == bSelected)
	{
		return;
	}

	bSelectedForRun = bSelected;
	if (SlotBounds)
	{
		SlotBounds->ShapeColor = bSelectedForRun ? FColor::Cyan : FColor(90, 90, 90);
	}

	OnSelectionChanged.Broadcast(this, bSelectedForRun);
}
