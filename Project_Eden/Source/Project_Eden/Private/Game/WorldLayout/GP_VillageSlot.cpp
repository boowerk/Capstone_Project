#include "Game/WorldLayout/GP_VillageSlot.h"

#include "Components/BoxComponent.h"
#include "EngineUtils.h"
#include "Game/WorldLayout/GP_VillageLayoutDirector.h"

AGP_VillageSlot::AGP_VillageSlot()
{
	PrimaryActorTick.bCanEverTick = false;

	SlotBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("SlotBounds"));
	SetRootComponent(SlotBounds);
	SlotBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SlotBounds->SetGenerateOverlapEvents(false);
	SlotBounds->SetBoxExtent(FVector(1.0f));
	SlotBounds->SetHiddenInGame(true);
	SlotBounds->SetVisibility(false, false);

	CapacityBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("CapacityBounds"));
	CapacityBounds->SetupAttachment(SlotBounds);
	CapacityBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CapacityBounds->SetGenerateOverlapEvents(false);
	CapacityBounds->SetHiddenInGame(true);
#if WITH_EDITORONLY_DATA
	CapacityBounds->SetIsVisualizationComponent(true);
#endif
	CapacityBounds->ShapeColor = FColor(255, 170, 0);

	FootprintBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("FootprintBounds"));
	FootprintBounds->SetupAttachment(SlotBounds);
	FootprintBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FootprintBounds->SetGenerateOverlapEvents(false);
	FootprintBounds->SetHiddenInGame(true);
#if WITH_EDITORONLY_DATA
	FootprintBounds->SetIsVisualizationComponent(true);
#endif
	UpdateCapacityPreview();
	ApplyFootprint(FGP_VillageFootprint());

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

void AGP_VillageSlot::Destroyed()
{
#if WITH_EDITOR
	if (GetWorld() && !GetWorld()->IsGameWorld())
	{
		for (TActorIterator<AGP_VillageLayoutDirector> It(GetWorld()); It; ++It)
		{
			It->RefreshFootprintPreviewIgnoringSlot(this);
		}
	}
#endif

	Super::Destroyed();
}

void AGP_VillageSlot::UpdateCapacityPreview()
{
	if (!CapacityBounds)
	{
		return;
	}

	const FVector2D CapacityHalfExtent = GPGetVillageSlotCapacityHalfExtent(SlotSizeClass);
	const FVector ActorScale = GetActorScale3D();
	const FVector InverseActorScale(
		FMath::IsNearlyZero(ActorScale.X) ? 1.0f : 1.0f / ActorScale.X,
		FMath::IsNearlyZero(ActorScale.Y) ? 1.0f : 1.0f / ActorScale.Y,
		FMath::IsNearlyZero(ActorScale.Z) ? 1.0f : 1.0f / ActorScale.Z);
	CapacityBounds->SetRelativeLocation(FVector::ZeroVector);
	CapacityBounds->SetRelativeScale3D(InverseActorScale);
	CapacityBounds->SetBoxExtent(
		FVector(CapacityHalfExtent.X, CapacityHalfExtent.Y, 1.0f),
		false);
	CapacityBounds->UpdateBounds();
	CapacityBounds->MarkRenderStateDirty();
}

void AGP_VillageSlot::ApplyFootprint(const FGP_VillageFootprint& Footprint)
{
	if (!FootprintBounds)
	{
		return;
	}

	constexpr float PreviewHalfHeight = 1.0f;
	const FVector PreviewOffset(
		Footprint.FootprintOffset.X,
		Footprint.FootprintOffset.Y,
		0.0f);
	const FVector PreviewExtent(
		FMath::Max(FMath::Abs(Footprint.FootprintExtent.X), 1.0f),
		FMath::Max(FMath::Abs(Footprint.FootprintExtent.Y), 1.0f),
		PreviewHalfHeight);
	const FVector ActorScale = GetActorScale3D();
	const FVector InverseActorScale(
		FMath::IsNearlyZero(ActorScale.X) ? 1.0f : 1.0f / ActorScale.X,
		FMath::IsNearlyZero(ActorScale.Y) ? 1.0f : 1.0f / ActorScale.Y,
		FMath::IsNearlyZero(ActorScale.Z) ? 1.0f : 1.0f / ActorScale.Z);
	// The footprint is an XY placement preview. Keep it flat at the slot origin so
	// editor floor snapping does not lift the runtime village origin into the air.
	// Level streaming and overlap tests intentionally use unit scale, so cancel any
	// authored slot scale while displaying that same XY contract.
	FootprintBounds->SetRelativeLocation(PreviewOffset * InverseActorScale);
	FootprintBounds->SetRelativeScale3D(InverseActorScale);
	FootprintBounds->SetBoxExtent(PreviewExtent, false);
	FootprintBounds->SetVisibility(true, false);
	FootprintBounds->UpdateBounds();
	UpdatePreviewColor();
}

void AGP_VillageSlot::ClearFootprintPreview()
{
	if (!FootprintBounds)
	{
		return;
	}

	bFootprintConflict = false;
	FootprintBounds->SetVisibility(false, false);
	FootprintBounds->MarkRenderStateDirty();
}

void AGP_VillageSlot::SetFootprintConflict(bool bConflicting)
{
	if (bFootprintConflict == bConflicting)
	{
		return;
	}

	bFootprintConflict = bConflicting;
	UpdatePreviewColor();
}

void AGP_VillageSlot::SetSelectedForRun(bool bSelected)
{
	if (bSelectedForRun == bSelected)
	{
		return;
	}

	bSelectedForRun = bSelected;
	UpdatePreviewColor();

	OnSelectionChanged.Broadcast(this, bSelectedForRun);
}

FColor AGP_VillageSlot::GetPreviewColor() const
{
	if (bFootprintConflict)
	{
		return FColor::Red;
	}
	return bSelectedForRun ? FColor::Cyan : FColor(90, 90, 90);
}

void AGP_VillageSlot::UpdatePreviewColor()
{
	if (!FootprintBounds)
	{
		return;
	}

	FootprintBounds->ShapeColor = GetPreviewColor();
	FootprintBounds->MarkRenderStateDirty();
}

#if WITH_EDITOR
void AGP_VillageSlot::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);
	if (bFinished)
	{
		NotifyLayoutDirectorsFootprintChanged();
	}
}

void AGP_VillageSlot::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	UpdateCapacityPreview();
	NotifyLayoutDirectorsFootprintChanged();
}

void AGP_VillageSlot::PostEditUndo()
{
	Super::PostEditUndo();
	UpdateCapacityPreview();
	NotifyLayoutDirectorsFootprintChanged();
}

void AGP_VillageSlot::NotifyLayoutDirectorsFootprintChanged() const
{
	UWorld* World = GetWorld();
	if (!World || World->IsGameWorld())
	{
		return;
	}

	for (TActorIterator<AGP_VillageLayoutDirector> It(World); It; ++It)
	{
		It->RefreshFootprintPreview();
	}
}
#endif
