#include "Game/WorldLayout/GP_VillageLayoutDirector.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Game/GP_GameState.h"
#include "Game/WorldLayout/GP_VillagePresetPolicy.h"
#include "Game/WorldLayout/GP_VillagePresetCatalog.h"
#include "Game/WorldLayout/GP_VillageSelectionPolicy.h"
#include "Game/WorldLayout/GP_VillageSlot.h"
#include "HAL/IConsoleManager.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
#if !UE_BUILD_SHIPPING
	TAutoConsoleVariable<int32> CVarVillageDebugDrawEnabled(
		TEXT("gp.Village.DebugDraw"),
		0,
		TEXT("Enables village selection debug drawing in non-Shipping builds."),
		ECVF_Cheat);
#endif
}

bool AGP_VillageLayoutDirector::IsDebugDrawingOptInEnabled()
{
#if UE_BUILD_SHIPPING
	return false;
#else
	return CVarVillageDebugDrawEnabled.GetValueOnGameThread() != 0;
#endif
}

int32 AGP_VillageLayoutDirector::SelectVillagePresetIndex(
	int32 InRunSeed,
	FName SlotId,
	const TArray<FGP_VillagePresetDefinition>& Presets,
	int32 AssignmentAttempt,
	EGP_VillageSlotSizeClass SlotSizeClass)
{
	return GPVillagePresetPolicy::SelectPresetIndex(
		InRunSeed,
		SlotId,
		Presets,
		AssignmentAttempt,
		SlotSizeClass);
}

TArray<FGP_VillagePresetDefinition> AGP_VillageLayoutDirector::MergeVillagePresetSources(
	const TArray<FGP_VillagePresetDefinition>& PrimaryPresets,
	const TArray<FGP_VillagePresetDefinition>& CatalogPresets,
	const TArray<FGP_VillagePresetDefinition>& LegacyPresets)
{
	return GPVillagePresetPolicy::MergePresetSources(
		PrimaryPresets,
		CatalogPresets,
		LegacyPresets);
}

bool AGP_VillageLayoutDirector::DoesVillageFootprintFitSlot(
	const FGP_VillageFootprint& Footprint,
	EGP_VillageSlotSizeClass SlotSizeClass)
{
	return GPVillagePresetPolicy::DoesFootprintFitSlot(Footprint, SlotSizeClass);
}

bool AGP_VillageLayoutDirector::DoVillageFootprintsOverlap(
	const FTransform& FirstSlotTransform,
	const FGP_VillageFootprint& FirstFootprint,
	const FTransform& SecondSlotTransform,
	const FGP_VillageFootprint& SecondFootprint)
{
	return GPVillagePresetPolicy::DoFootprintsOverlap(
		FirstSlotTransform,
		FirstFootprint,
		SecondSlotTransform,
		SecondFootprint);
}

AGP_VillageLayoutDirector::AGP_VillageLayoutDirector()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	FGP_VillageGroupRule DefaultRule;
	GroupRules.Add(DefaultRule);

	VillageLevelPreset = TSoftObjectPtr<UWorld>(
		FSoftObjectPath(TEXT("/Game/WorldLayout/L_Village_00.L_Village_00")));
	static ConstructorHelpers::FObjectFinder<UGP_VillagePresetCatalog>
		DefaultPresetCatalogFinder(
			TEXT("/Game/WorldLayout/DA_VillagePresetCatalog.DA_VillagePresetCatalog"));
	VillagePresetCatalog = DefaultPresetCatalogFinder.Object;

#if WITH_EDITORONLY_DATA
	bIsSpatiallyLoaded = false;
#endif
}

void AGP_VillageLayoutDirector::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGP_VillageLayoutDirector, RuntimeLayoutSnapshot);
}

TArray<FGP_VillagePresetDefinition> AGP_VillageLayoutDirector::GetVillagePresetPool() const
{
	return BuildEffectiveVillagePresetPool();
}

TArray<FGP_VillagePresetDefinition> AGP_VillageLayoutDirector::BuildEffectiveVillagePresetPool() const
{
	TArray<FGP_VillagePresetDefinition> PrimaryPresets;
	if (!VillageLevelPreset.IsNull())
	{
		FGP_VillagePresetDefinition PrimaryPreset;
		PrimaryPreset.PresetId = TEXT("Village_00");
		PrimaryPreset.VillageLevel = VillageLevelPreset;
		PrimaryPreset.PresetSizeClass = EGP_VillageSlotSizeClass::Medium;
		PrimaryPreset.Footprint = VillagePresetFootprint;
		PrimaryPresets.Add(PrimaryPreset);
	}

	const TArray<FGP_VillagePresetDefinition> EmptyCatalog;
	const TArray<FGP_VillagePresetDefinition>& CatalogPresets =
		VillagePresetCatalog ? VillagePresetCatalog->Presets : EmptyCatalog;
	return GPVillagePresetPolicy::MergePresetSources(
		PrimaryPresets,
		CatalogPresets,
		VillagePresetPool);
}

void AGP_VillageLayoutDirector::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (GetWorld() && !GetWorld()->IsGameWorld())
	{
		RefreshFootprintPreview();
	}
}

void AGP_VillageLayoutDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ClientVisualReadyRetryHandle);
	}
	UnloadActiveVillageLevels();
	Super::EndPlay(EndPlayReason);
}

void AGP_VillageLayoutDirector::Destroyed()
{
	ClearEditorVillagePreview();
	Super::Destroyed();
}

void AGP_VillageLayoutDirector::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		TryApplyReplicatedRuntimeLayout();
		return;
	}

	if (!bAutoBuildOnBeginPlay)
	{
		return;
	}

#if WITH_EDITOR
	if (GetWorld() && bUsePreviewSeedInPIE && GetWorld()->IsPlayInEditor())
	{
		const AGP_GameState* PIEGameState = GetWorld()->GetGameState<AGP_GameState>();
		const int32 PIEGameStateSeed =
			PIEGameState && PIEGameState->HasRunSeed()
			? PIEGameState->GetRunSeed()
			: INDEX_NONE;
		UE_LOG(LogTemp, Log,
			TEXT("[VillageLayout] PIE seed override active: PreviewSeed=%d (GameState RunSeed=%d)."),
			PreviewSeed,
			PIEGameStateSeed);
		BuildSelectionForSeed(PreviewSeed);
		return;
	}
#endif

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

	bRuntimeLayoutReady = false;
	CollectSlots();
	TArray<AGP_VillageSlot*> CandidateSlots;
	for (AGP_VillageSlot* Slot : CachedSlots)
	{
		if (IsValid(Slot) && Slot->IsCandidateEnabled())
		{
			CandidateSlots.Add(Slot);
		}
	}

	FGP_VillageSelectionResult Result;
	TArray<FGP_VillageCandidate> Candidates;
	TMap<FName, int32> BestPresetAssignments;
	bool bHasSelectionAttempt = false;
	const int32 AssignmentAttemptCount = BuildEffectiveVillagePresetPool().Num() > 1 ? 32 : 1;
	for (int32 AssignmentAttempt = 0; AssignmentAttempt < AssignmentAttemptCount; ++AssignmentAttempt)
	{
		AssignVillagePresets(InRunSeed, AssignmentAttempt);
		TArray<AGP_VillageSlot*> AttemptCandidateSlots;
		TArray<FGP_VillageCandidate> AttemptCandidates;
		for (AGP_VillageSlot* Slot : CandidateSlots)
		{
			const int32* AssignedPresetIndex =
				AssignedVillagePresetIndices.Find(Slot->GetSlotId());
			if (!AssignedPresetIndex || *AssignedPresetIndex == INDEX_NONE)
			{
				continue;
			}

			AttemptCandidateSlots.Add(Slot);
			AttemptCandidates.Add(Slot->MakeCandidate());
		}
		PopulateCandidateOverlaps(AttemptCandidates, AttemptCandidateSlots);

		FGP_VillageSelectionResult AttemptResult =
			GPVillageSelectionPolicy::SelectSlots(InRunSeed, AttemptCandidates, GroupRules);
		const bool bAttemptIsBetter = !bHasSelectionAttempt
			|| (AttemptResult.bSucceeded && !Result.bSucceeded)
			|| (AttemptResult.bSucceeded == Result.bSucceeded
				&& AttemptResult.SelectedSlotIds.Num() > Result.SelectedSlotIds.Num());
		if (bAttemptIsBetter)
		{
			Result = MoveTemp(AttemptResult);
			Candidates = MoveTemp(AttemptCandidates);
			BestPresetAssignments = AssignedVillagePresetIndices;
			LastPresetAssignmentAttempt = AssignmentAttempt;
			bHasSelectionAttempt = true;
		}
	}

	AssignedVillagePresetIndices = MoveTemp(BestPresetAssignments);
	ApplyFootprintPreview();
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

	const bool bIsGameWorld = GetWorld() && GetWorld()->IsGameWorld();
	const bool bShouldApplyDataLayers = bIsGameWorld && bApplyDataLayersAtRuntime;
	const bool bDataLayersApplied = !bShouldApplyDataLayers || ApplyDataLayerStates(SelectedSet);
	const bool bShouldStreamVillageLevel = bIsGameWorld && bStreamVillageLevelAtRuntime;
	const bool bVillageLevelScheduled = !bShouldStreamVillageLevel
		|| (Result.bSucceeded && StreamSelectedVillageLevels(SelectedSet));
	if (bIsGameWorld
		&& HasAuthority()
		&& (!bShouldStreamVillageLevel || !Result.bSucceeded))
	{
		PublishEmptyRuntimeLayoutSnapshot();
	}

	TArray<FString> SelectedNames;
	for (FName SlotId : SelectedSlotIds)
	{
		TSoftObjectPtr<UWorld> AssignedLevel;
		FGP_VillageFootprint AssignedFootprint;
		FName AssignedPresetId;
		ResolveVillagePreset(SlotId, AssignedLevel, AssignedFootprint, AssignedPresetId);
		SelectedNames.Add(FString::Printf(
			TEXT("%s=%s"),
			*SlotId.ToString(),
			AssignedPresetId.IsNone() ? TEXT("None") : *AssignedPresetId.ToString()));
	}

	const TCHAR* DataLayerStatus = !bIsGameWorld
		? TEXT("Preview")
		: (!bApplyDataLayersAtRuntime ? TEXT("Disabled") : (bDataLayersApplied ? TEXT("Applied") : TEXT("Failed")));
	const TCHAR* LevelInstanceStatus = !bIsGameWorld
		? TEXT("Preview")
		: (!bStreamVillageLevelAtRuntime ? TEXT("Disabled") : (bVillageLevelScheduled ? TEXT("Scheduled") : TEXT("Failed")));
	LastSelectionSummary = FString::Printf(
		TEXT("Seed=%d PresetAttempt=%d Candidates=%d Selected=[%s] DataLayers=%s LevelInstance=%s"),
		InRunSeed,
		LastPresetAssignmentAttempt,
		Candidates.Num(),
		*FString::Join(SelectedNames, TEXT(", ")),
		DataLayerStatus,
		LevelInstanceStatus);

	UE_LOG(LogTemp, Log, TEXT("[VillageLayout] %s"), *LastSelectionSummary);
	for (const FString& Warning : Result.Warnings)
	{
		UE_LOG(LogTemp, Warning, TEXT("[VillageLayout] %s"), *Warning);
	}

	if (bDrawDebug && IsDebugDrawingOptInEnabled())
	{
		DrawSelectionDebug();
	}

	if (bIsGameWorld && (!bShouldStreamVillageLevel || SelectedSet.IsEmpty()))
	{
		MarkRuntimeLayoutReady();
	}

	return Result.bSucceeded && bDataLayersApplied && bVillageLevelScheduled;
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

void AGP_VillageLayoutDirector::AssignVillagePresets(int32 InRunSeed, int32 AssignmentAttempt)
{
	AssignedVillagePresetIndices.Reset();
	const TArray<FGP_VillagePresetDefinition> EffectivePresets = BuildEffectiveVillagePresetPool();
	for (const AGP_VillageSlot* Slot : CachedSlots)
	{
		if (!IsValid(Slot))
		{
			continue;
		}

		const int32 PresetIndex = GPVillagePresetPolicy::SelectPresetIndex(
			InRunSeed,
			Slot->GetSlotId(),
			EffectivePresets,
			AssignmentAttempt,
			Slot->GetSlotSizeClass());
		AssignedVillagePresetIndices.Add(Slot->GetSlotId(), PresetIndex);
	}
}

bool AGP_VillageLayoutDirector::ResolveVillagePreset(
	FName SlotId,
	TSoftObjectPtr<UWorld>& OutLevel,
	FGP_VillageFootprint& OutFootprint,
	FName& OutPresetId) const
{
	const TArray<FGP_VillagePresetDefinition> EffectivePresets = BuildEffectiveVillagePresetPool();
	if (const int32* PresetIndex = AssignedVillagePresetIndices.Find(SlotId);
		PresetIndex)
	{
		if (EffectivePresets.IsValidIndex(*PresetIndex))
		{
			const FGP_VillagePresetDefinition& Preset = EffectivePresets[*PresetIndex];
			if (!Preset.VillageLevel.IsNull() && Preset.SelectionWeight > 0.0f)
			{
				OutLevel = Preset.VillageLevel;
				OutFootprint = Preset.Footprint;
				OutPresetId = Preset.PresetId.IsNone()
					? FName(*Preset.VillageLevel.ToSoftObjectPath().GetAssetName())
					: Preset.PresetId;
				return true;
			}
		}

		OutLevel.Reset();
		OutFootprint = FGP_VillageFootprint();
		OutPresetId = NAME_None;
		return false;
	}

	OutLevel = VillageLevelPreset;
	OutFootprint = VillagePresetFootprint;
	OutPresetId = VillageLevelPreset.IsNull()
		? NAME_None
		: FName(*VillageLevelPreset.ToSoftObjectPath().GetAssetName());
	return !OutLevel.IsNull();
}

void AGP_VillageLayoutDirector::ApplyFootprintPreview()
{
	for (AGP_VillageSlot* Slot : CachedSlots)
	{
		if (!IsValid(Slot))
		{
			continue;
		}

		TSoftObjectPtr<UWorld> AssignedLevel;
		FGP_VillageFootprint AssignedFootprint;
		FName AssignedPresetId;
		if (!ResolveVillagePreset(
			Slot->GetSlotId(),
			AssignedLevel,
			AssignedFootprint,
			AssignedPresetId))
		{
			Slot->ClearFootprintPreview();
			continue;
		}
		Slot->ApplyFootprint(AssignedFootprint);
		Slot->SetFootprintConflict(false);
	}

	for (int32 FirstIndex = 0; FirstIndex < CachedSlots.Num(); ++FirstIndex)
	{
		AGP_VillageSlot* FirstSlot = CachedSlots[FirstIndex];
		if (!IsValid(FirstSlot) || !FirstSlot->IsCandidateEnabled())
		{
			continue;
		}

		for (int32 SecondIndex = FirstIndex + 1; SecondIndex < CachedSlots.Num(); ++SecondIndex)
		{
			AGP_VillageSlot* SecondSlot = CachedSlots[SecondIndex];
			if (!IsValid(SecondSlot) || !SecondSlot->IsCandidateEnabled())
			{
				continue;
			}

			TSoftObjectPtr<UWorld> FirstLevel;
			TSoftObjectPtr<UWorld> SecondLevel;
			FGP_VillageFootprint FirstFootprint;
			FGP_VillageFootprint SecondFootprint;
			FName FirstPresetId;
			FName SecondPresetId;
			if (!ResolveVillagePreset(
				FirstSlot->GetSlotId(),
				FirstLevel,
				FirstFootprint,
				FirstPresetId)
				|| !ResolveVillagePreset(
				SecondSlot->GetSlotId(),
				SecondLevel,
				SecondFootprint,
				SecondPresetId))
			{
				continue;
			}
			if (GPVillagePresetPolicy::DoFootprintsOverlap(
				FirstSlot->GetActorTransform(),
				FirstFootprint,
				SecondSlot->GetActorTransform(),
				SecondFootprint))
			{
				FirstSlot->SetFootprintConflict(true);
				SecondSlot->SetFootprintConflict(true);
			}
		}
	}
}

void AGP_VillageLayoutDirector::PopulateCandidateOverlaps(
	TArray<FGP_VillageCandidate>& InOutCandidates,
	const TArray<AGP_VillageSlot*>& CandidateSlots) const
{
	check(InOutCandidates.Num() == CandidateSlots.Num());

	for (int32 FirstIndex = 0; FirstIndex < CandidateSlots.Num(); ++FirstIndex)
	{
		const AGP_VillageSlot* FirstSlot = CandidateSlots[FirstIndex];
		if (!IsValid(FirstSlot))
		{
			continue;
		}

		for (int32 SecondIndex = FirstIndex + 1; SecondIndex < CandidateSlots.Num(); ++SecondIndex)
		{
			const AGP_VillageSlot* SecondSlot = CandidateSlots[SecondIndex];
			if (!IsValid(SecondSlot))
			{
				continue;
			}

			TSoftObjectPtr<UWorld> FirstLevel;
			TSoftObjectPtr<UWorld> SecondLevel;
			FGP_VillageFootprint FirstFootprint;
			FGP_VillageFootprint SecondFootprint;
			FName FirstPresetId;
			FName SecondPresetId;
			ResolveVillagePreset(
				FirstSlot->GetSlotId(),
				FirstLevel,
				FirstFootprint,
				FirstPresetId);
			ResolveVillagePreset(
				SecondSlot->GetSlotId(),
				SecondLevel,
				SecondFootprint,
				SecondPresetId);
			if (!GPVillagePresetPolicy::DoFootprintsOverlap(
				FirstSlot->GetActorTransform(),
				FirstFootprint,
				SecondSlot->GetActorTransform(),
				SecondFootprint))
			{
				continue;
			}

			InOutCandidates[FirstIndex].OverlappingSlotIds.AddUnique(InOutCandidates[SecondIndex].SlotId);
			InOutCandidates[SecondIndex].OverlappingSlotIds.AddUnique(InOutCandidates[FirstIndex].SlotId);
		}
	}
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
		const FColor Color = Slot->GetPreviewColor();
		TSoftObjectPtr<UWorld> AssignedLevel;
		FGP_VillageFootprint AssignedFootprint;
		FName AssignedPresetId;
		if (!ResolveVillagePreset(
			Slot->GetSlotId(),
			AssignedLevel,
			AssignedFootprint,
			AssignedPresetId))
		{
			continue;
		}
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
			TEXT("%s / %s / %s / %s%s"),
			*Slot->GetGroupId().ToString(),
			*Slot->GetSlotId().ToString(),
			AssignedPresetId.IsNone() ? TEXT("NoPreset") : *AssignedPresetId.ToString(),
			Slot->IsSelectedForRun() ? TEXT("SELECTED") : TEXT("OFF"),
			Slot->HasFootprintConflict() ? TEXT(" / OVERLAP") : TEXT(""));
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
