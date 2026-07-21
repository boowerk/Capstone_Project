#include "Game/WorldLayout/GP_VillageLayoutDirector.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "Game/GP_GameState.h"
#include "Game/WorldLayout/GP_VillageSelectionPolicy.h"
#include "Game/WorldLayout/GP_VillageSlot.h"

AGP_VillageLayoutDirector::AGP_VillageLayoutDirector()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	FGP_VillageGroupRule DefaultRule;
	GroupRules.Add(DefaultRule);

#if WITH_EDITORONLY_DATA
	bIsSpatiallyLoaded = false;
#endif
}

void AGP_VillageLayoutDirector::BeginPlay()
{
	Super::BeginPlay();

	if (!bAutoBuildOnBeginPlay || !HasAuthority())
	{
		return;
	}

	const AGP_GameState* GPGameState = GetWorld() ? GetWorld()->GetGameState<AGP_GameState>() : nullptr;
	if (!GPGameState || !GPGameState->HasRunSeed())
	{
		UE_LOG(LogTemp, Error, TEXT("[VillageLayout] Cannot build selection because RunSeed is unavailable."));
		return;
	}

	BuildSelectionForSeed(GPGameState->GetRunSeed());
}

bool AGP_VillageLayoutDirector::BuildSelectionForSeed(int32 InRunSeed)
{
	if (GetWorld() && GetWorld()->IsGameWorld() && !HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[VillageLayout] Only the server can build a runtime village selection."));
		return false;
	}

	CollectSlots();

	TArray<FGP_VillageCandidate> Candidates;
	for (AGP_VillageSlot* Slot : CachedSlots)
	{
		if (IsValid(Slot) && Slot->IsCandidateEnabled())
		{
			Candidates.Add(Slot->MakeCandidate());
		}
	}

	const FGP_VillageSelectionResult Result = GPVillageSelectionPolicy::SelectSlots(InRunSeed, Candidates, GroupRules);
	SelectedSlotIds = Result.bSucceeded ? Result.SelectedSlotIds : TArray<FName>();
	LastRunSeed = InRunSeed;

	TSet<FName> SelectedSet;
	for (FName SlotId : SelectedSlotIds)
	{
		SelectedSet.Add(SlotId);
	}
	for (AGP_VillageSlot* Slot : CachedSlots)
	{
		if (IsValid(Slot))
		{
			Slot->SetSelectedForRun(SelectedSet.Contains(Slot->GetSlotId()));
		}
	}

	TArray<FString> SelectedNames;
	for (FName SlotId : SelectedSlotIds)
	{
		SelectedNames.Add(SlotId.ToString());
	}

	LastSelectionSummary = FString::Printf(
		TEXT("Seed=%d Candidates=%d Selected=[%s]"),
		InRunSeed,
		Candidates.Num(),
		*FString::Join(SelectedNames, TEXT(", ")));

	UE_LOG(LogTemp, Log, TEXT("[VillageLayout] %s"), *LastSelectionSummary);
	for (const FString& Warning : Result.Warnings)
	{
		UE_LOG(LogTemp, Warning, TEXT("[VillageLayout] %s"), *Warning);
	}

	if (bDrawDebug)
	{
		DrawSelectionDebug();
	}

	return Result.bSucceeded;
}

void AGP_VillageLayoutDirector::RebuildPreview()
{
	BuildSelectionForSeed(PreviewSeed);
}

void AGP_VillageLayoutDirector::CollectSlots()
{
	CachedSlots.Reset();
	if (!GetWorld())
	{
		return;
	}

	for (TActorIterator<AGP_VillageSlot> It(GetWorld()); It; ++It)
	{
		CachedSlots.Add(*It);
	}

	CachedSlots.Sort([](const AGP_VillageSlot& A, const AGP_VillageSlot& B)
	{
		return A.GetSlotId().LexicalLess(B.GetSlotId());
	});
}

void AGP_VillageLayoutDirector::DrawSelectionDebug() const
{
	if (!GetWorld())
	{
		return;
	}

	for (const AGP_VillageSlot* Slot : CachedSlots)
	{
		if (!IsValid(Slot) || !Slot->GetSlotBounds())
		{
			continue;
		}

		const UBoxComponent* Bounds = Slot->GetSlotBounds();
		const FColor Color = Slot->IsSelectedForRun() ? FColor::Cyan : FColor(90, 90, 90);
		DrawDebugBox(
			GetWorld(),
			Bounds->GetComponentLocation(),
			Bounds->GetScaledBoxExtent(),
			Bounds->GetComponentQuat(),
			Color,
			false,
			DebugDrawDuration,
			0,
			20.0f);

		const FString Label = FString::Printf(
			TEXT("%s / %s / %s"),
			*Slot->GetGroupId().ToString(),
			*Slot->GetSlotId().ToString(),
			Slot->IsSelectedForRun() ? TEXT("SELECTED") : TEXT("OFF"));
		DrawDebugString(
			GetWorld(),
			Bounds->GetComponentLocation() + FVector(0.0f, 0.0f, Bounds->GetScaledBoxExtent().Z + 200.0f),
			Label,
			nullptr,
			Color,
			DebugDrawDuration,
			true,
			1.2f);
	}
}
