#include "Game/WorldLayout/GP_VillageLayoutDirector.h"

#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Level.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Game/GP_EnemySpawnVolume.h"
#include "Game/GP_GameState.h"
#include "Game/WorldLayout/GP_VillagePresetPolicy.h"
#include "Game/WorldLayout/GP_VillagePresetCatalog.h"
#include "Game/WorldLayout/GP_VillageSelectionPolicy.h"
#include "Game/WorldLayout/GP_VillageSlot.h"
#include "Misc/Crc.h"
#include "Net/UnrealNetwork.h"
#include "PCGComponent.h"
#include "PCGGraph.h"
#include "PCGManagedResource.h"
#include "Player/GP_PlayerController.h"
#include "UObject/ConstructorHelpers.h"
#include "WorldPartition/DataLayer/DataLayerAsset.h"
#include "WorldPartition/DataLayer/DataLayerManager.h"

namespace
{
	constexpr TCHAR CityPCGGraphObjectPath[] =
		TEXT("/Game/RegionSystem/PCG/CityGen/PCG_CityGen_FromAnchor.PCG_CityGen_FromAnchor");

	struct FGPVillageDataLayerBinding
	{
		const AGP_VillageSlot* Slot = nullptr;
		const UDataLayerAsset* Asset = nullptr;
		const UDataLayerInstance* Instance = nullptr;
	};

	EGPZoneStage ResolveZoneStage(FName GroupId, FName SlotId)
	{
		const FString Group = GroupId.ToString();
		const FString Slot = SlotId.ToString();
		if (Group.Equals(TEXT("Outer"), ESearchCase::IgnoreCase)
			|| Slot.StartsWith(TEXT("Outer_"), ESearchCase::IgnoreCase))
		{
			return EGPZoneStage::Outer;
		}
		if (Group.Equals(TEXT("Middle"), ESearchCase::IgnoreCase)
			|| Slot.StartsWith(TEXT("Middle_"), ESearchCase::IgnoreCase))
		{
			return EGPZoneStage::Middle;
		}
		if (Group.Equals(TEXT("Center"), ESearchCase::IgnoreCase)
			|| Slot.StartsWith(TEXT("Center_"), ESearchCase::IgnoreCase))
		{
			return EGPZoneStage::Center;
		}
		if (Group.Equals(TEXT("Colosseum"), ESearchCase::IgnoreCase)
			|| Slot.StartsWith(TEXT("Colosseum_"), ESearchCase::IgnoreCase))
		{
			return EGPZoneStage::Colosseum;
		}
		return EGPZoneStage::Legacy;
	}

	int32 GetZoneStageOrder(EGPZoneStage Stage)
	{
		switch (Stage)
		{
		case EGPZoneStage::Outer:
			return 0;
		case EGPZoneStage::Middle:
			return 1;
		case EGPZoneStage::Center:
			return 2;
		case EGPZoneStage::Colosseum:
			return 3;
		case EGPZoneStage::Legacy:
		default:
			return 0;
		}
	}
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

	if (bDrawDebug)
	{
		DrawSelectionDebug();
	}

	if (bIsGameWorld && (!bShouldStreamVillageLevel || SelectedSet.IsEmpty()))
	{
		MarkRuntimeLayoutReady();
	}

	return Result.bSucceeded && bDataLayersApplied && bVillageLevelScheduled;
}

UPCGComponent* AGP_VillageLayoutDirector::FindVillagePCGComponent(
	ULevelStreamingDynamic* StreamingLevel) const
{
	ULevel* LoadedLevel = StreamingLevel ? StreamingLevel->GetLoadedLevel() : nullptr;
	if (!LoadedLevel)
	{
		return nullptr;
	}

	UPCGComponent* Result = nullptr;
	for (AActor* Actor : LoadedLevel->Actors)
	{
		if (!IsValid(Actor))
		{
			continue;
		}

		TInlineComponentArray<UPCGComponent*> PCGComponents(Actor);
		for (UPCGComponent* PCGComponent : PCGComponents)
		{
			FName RoadTag;
			FName DistrictTag;
			if (!GetCityPCGTemplateTags(PCGComponent, RoadTag, DistrictTag))
			{
				continue;
			}

			if (Result)
			{
				return nullptr;
			}
			Result = PCGComponent;
		}
	}
	return Result;
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

bool AGP_VillageLayoutDirector::ApplyDataLayerStates(const TSet<FName>& InSelectedSlotIds) const
{
	UDataLayerManager* DataLayerManager = UDataLayerManager::GetDataLayerManager(this);
	if (!DataLayerManager)
	{
		UE_LOG(LogTemp, Error, TEXT("[VillageLayout] Runtime Data Layer application requires a World Partition DataLayerManager."));
		return false;
	}

	TArray<FGPVillageDataLayerBinding> Bindings;
	TSet<const UDataLayerAsset*> SeenAssets;
	bool bConfigurationValid = true;

	for (const AGP_VillageSlot* Slot : CachedSlots)
	{
		if (!IsValid(Slot))
		{
			continue;
		}

		const UDataLayerAsset* Asset = Slot->GetVillageDataLayer();
		if (!Asset)
		{
			if (Slot->IsCandidateEnabled())
			{
				UE_LOG(LogTemp, Error,
					TEXT("[VillageLayout] Enabled slot '%s' has no VillageDataLayer assigned."),
					*Slot->GetSlotId().ToString());
				bConfigurationValid = false;
			}
			continue;
		}

		if (SeenAssets.Contains(Asset))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[VillageLayout] Data Layer '%s' is assigned to more than one village slot."),
				*Asset->GetPathName());
			bConfigurationValid = false;
			continue;
		}
		SeenAssets.Add(Asset);

		const UDataLayerInstance* Instance = DataLayerManager->GetDataLayerInstanceFromAsset(Asset);
		if (!Asset->IsRuntime() || !Instance || !Instance->IsRuntime())
		{
			UE_LOG(LogTemp, Error,
				TEXT("[VillageLayout] Slot '%s' Data Layer '%s' must be a Runtime Data Layer instance in this world."),
				*Slot->GetSlotId().ToString(),
				*Asset->GetPathName());
			bConfigurationValid = false;
			continue;
		}

		if (Instance->GetInitialRuntimeState() != EDataLayerRuntimeState::Unloaded)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[VillageLayout] Slot '%s' Data Layer '%s' must start Unloaded."),
				*Slot->GetSlotId().ToString(),
				*Asset->GetPathName());
			bConfigurationValid = false;
		}

		Bindings.Add({Slot, Asset, Instance});
	}

	// Fail closed: never leave candidate content active when the mapping is incomplete.
	for (const FGPVillageDataLayerBinding& Binding : Bindings)
	{
		if (!DataLayerManager->SetDataLayerInstanceRuntimeState(
			Binding.Instance,
			EDataLayerRuntimeState::Unloaded,
			false))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[VillageLayout] Failed to unload Data Layer '%s'."),
				*Binding.Asset->GetPathName());
			bConfigurationValid = false;
		}
	}

	if (!bConfigurationValid)
	{
		return false;
	}

	for (const FGPVillageDataLayerBinding& Binding : Bindings)
	{
		if (!Binding.Slot->IsCandidateEnabled() || !InSelectedSlotIds.Contains(Binding.Slot->GetSlotId()))
		{
			continue;
		}

		if (!DataLayerManager->SetDataLayerInstanceRuntimeState(
			Binding.Instance,
			EDataLayerRuntimeState::Activated,
			false))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[VillageLayout] Failed to activate selected Data Layer '%s' for slot '%s'."),
				*Binding.Asset->GetPathName(),
				*Binding.Slot->GetSlotId().ToString());

			for (const FGPVillageDataLayerBinding& RollbackBinding : Bindings)
			{
				DataLayerManager->SetDataLayerInstanceRuntimeState(
					RollbackBinding.Instance,
					EDataLayerRuntimeState::Unloaded,
					false);
			}
			return false;
		}

		UE_LOG(LogTemp, Log,
			TEXT("[VillageLayout] Activated Data Layer '%s' for slot '%s'."),
			*Binding.Asset->GetPathName(),
			*Binding.Slot->GetSlotId().ToString());
	}

	return true;
}

bool AGP_VillageLayoutDirector::StreamSelectedVillageLevels(const TSet<FName>& InSelectedSlotIds)
{
	TArray<FGP_VillageRuntimeInstance> Instances;
	if (!BuildRuntimeLayoutInstances(InSelectedSlotIds, Instances))
	{
		PublishEmptyRuntimeLayoutSnapshot();
		UnloadActiveVillageLevels();
		return false;
	}

	// Publish before server-side streaming so clients can load and generate their
	// visual PCG in parallel with the dedicated server.
	PublishRuntimeLayoutSnapshot(Instances, true);
	if (StreamRuntimeVillageLevels(Instances))
	{
		return true;
	}

	PublishEmptyRuntimeLayoutSnapshot();
	return false;
}

bool AGP_VillageLayoutDirector::BuildRuntimeLayoutInstances(
	const TSet<FName>& InSelectedSlotIds,
	TArray<FGP_VillageRuntimeInstance>& OutInstances) const
{
	OutInstances.Reset();
	OutInstances.Reserve(InSelectedSlotIds.Num());
	const int32 SnapshotRevision = GetNextRuntimeLayoutRevision();
	for (const AGP_VillageSlot* Slot : CachedSlots)
	{
		if (!IsValid(Slot) || !InSelectedSlotIds.Contains(Slot->GetSlotId()))
		{
			continue;
		}

		const FName SlotId = Slot->GetSlotId();
		TSoftObjectPtr<UWorld> AssignedLevel;
		FGP_VillageFootprint AssignedFootprint;
		FName AssignedPresetId;
		if (!ResolveVillagePreset(SlotId, AssignedLevel, AssignedFootprint, AssignedPresetId))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[VillageLayout] No valid village preset is assigned to slot '%s'."),
				*SlotId.ToString());
			OutInstances.Reset();
			return false;
		}

		FGP_VillageRuntimeInstance& Instance = OutInstances.AddDefaulted_GetRef();
		Instance.SlotId = SlotId;
		Instance.PresetId = AssignedPresetId;
		Instance.VillageLevel = AssignedLevel;
		Instance.SpawnTransform = FTransform(
			Slot->GetActorRotation(),
			Slot->GetActorLocation(),
			FVector::OneVector);
		Instance.StableInstanceName = FName(*FString::Printf(
			TEXT("%s_R%08X"),
			*MakeStreamingLevelName(SlotId),
			static_cast<uint32>(SnapshotRevision)));
		Instance.PCGSeed = MakeVillagePCGSeed(SlotId);
	}

	if (OutInstances.Num() != InSelectedSlotIds.Num())
	{
		UE_LOG(LogTemp, Error,
			TEXT("[VillageLayout] Built %d of %d selected runtime village instances."),
			OutInstances.Num(),
			InSelectedSlotIds.Num());
		OutInstances.Reset();
		return false;
	}

	return true;
}

bool AGP_VillageLayoutDirector::StreamRuntimeVillageLevels(
	const TArray<FGP_VillageRuntimeInstance>& Instances)
{
	UnloadActiveVillageLevels();
	bVillageLevelBatchFailed = false;

	if (Instances.IsEmpty())
	{
		MarkRuntimeLayoutReady();
		return true;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	TSet<FName> SeenSlotIds;
	TSet<FName> SeenInstanceNames;
	for (const FGP_VillageRuntimeInstance& Instance : Instances)
	{
		if (Instance.SlotId.IsNone()
			|| Instance.VillageLevel.IsNull()
			|| Instance.StableInstanceName.IsNone()
			|| Instance.PCGSeed < 0
			|| Instance.SpawnTransform.ContainsNaN()
			|| SeenSlotIds.Contains(Instance.SlotId)
			|| SeenInstanceNames.Contains(Instance.StableInstanceName))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[VillageLayout] Invalid or duplicate runtime village instance Slot='%s' Preset='%s' Level='%s' Name='%s' Seed=%d."),
				*Instance.SlotId.ToString(),
				*Instance.PresetId.ToString(),
				*Instance.VillageLevel.ToString(),
				*Instance.StableInstanceName.ToString(),
				Instance.PCGSeed);
			UnloadActiveVillageLevels();
			return false;
		}

		SeenSlotIds.Add(Instance.SlotId);
		SeenInstanceNames.Add(Instance.StableInstanceName);
	}

	ExpectedVillageLevelCount = Instances.Num();
	int32 ScheduledLevelCount = 0;
	for (const FGP_VillageRuntimeInstance& Instance : Instances)
	{
		const FString StableLevelName = Instance.StableInstanceName.ToString();
		ULevelStreamingDynamic::FLoadLevelInstanceParams Params(
			World,
			Instance.VillageLevel.ToSoftObjectPath().GetLongPackageName(),
			Instance.SpawnTransform);
		Params.OptionalLevelNameOverride = &StableLevelName;
		Params.bInitiallyVisible = false;
		Params.LevelStreamingCreatedCallback = [this](ULevelStreaming* CreatedStreamingLevel)
		{
			if (CreatedStreamingLevel)
			{
				CreatedStreamingLevel->OnLevelLoaded.AddUniqueDynamic(
					this,
					&AGP_VillageLayoutDirector::HandleVillageLevelLoaded);
				CreatedStreamingLevel->OnLevelShown.AddUniqueDynamic(
					this,
					&AGP_VillageLayoutDirector::HandleVillageLevelShown);
			}
		};

		bool bLoadScheduled = false;
		ULevelStreamingDynamic* StreamingLevel = ULevelStreamingDynamic::LoadLevelInstance(Params, bLoadScheduled);
		if (!bLoadScheduled || !StreamingLevel)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[VillageLayout] Failed to schedule village preset '%s' (%s) for slot '%s'."),
				*Instance.PresetId.ToString(),
				*Instance.VillageLevel.ToString(),
				*Instance.SlotId.ToString());
			UnloadActiveVillageLevels();
			return false;
		}

		ActiveVillageLevels.Add(Instance.SlotId, StreamingLevel);
		ActiveVillagePCGSeeds.Add(Instance.SlotId, Instance.PCGSeed);
		ActiveVillageInstanceNames.Add(Instance.SlotId, Instance.StableInstanceName);
		++ScheduledLevelCount;
		UE_LOG(LogTemp, Log,
			TEXT("[VillageLayout] %s scheduled village preset '%s' (%s) at slot '%s' Location=%s Instance='%s' PCGSeed=%d."),
			HasAuthority() ? TEXT("Server") : TEXT("Client"),
			*Instance.PresetId.ToString(),
			*Instance.VillageLevel.ToString(),
			*Instance.SlotId.ToString(),
			*Instance.SpawnTransform.GetLocation().ToCompactString(),
			*Instance.StableInstanceName.ToString(),
			Instance.PCGSeed);
	}

	if (ScheduledLevelCount != Instances.Num())
	{
		UE_LOG(LogTemp, Error,
			TEXT("[VillageLayout] Scheduled %d of %d selected village slots; rolling back."),
			ScheduledLevelCount,
			Instances.Num());
		UnloadActiveVillageLevels();
		return false;
	}

	// Covers synchronous/cached loads where delegates can fire before every map entry is stored.
	HandleVillageLevelLoaded();
	HandleVillageLevelShown();
	if (!bVillageLevelBatchFailed
		&& ExpectedVillageLevelCount > 0
		&& GeneratedVillageLevelIds.Num() < ExpectedVillageLevelCount)
	{
		// A slow client must be allowed to finish local visual PCG. The server
		// retains the authoritative timeout and publishes an unload snapshot if
		// its gameplay layout cannot finish.
		if (HasAuthority())
		{
			World->GetTimerManager().SetTimer(
				VillageStreamingTimeoutHandle,
				this,
				&AGP_VillageLayoutDirector::HandleVillageStreamingTimeout,
				FMath::Max(1.0f, VillageStreamingTimeoutSeconds),
				false);
		}
	}

	return !bVillageLevelBatchFailed;
}

void AGP_VillageLayoutDirector::PublishRuntimeLayoutSnapshot(
	const TArray<FGP_VillageRuntimeInstance>& Instances,
	bool bShouldLoad)
{
	if (!HasAuthority() || !GetWorld() || !GetWorld()->IsGameWorld())
	{
		return;
	}

	FGP_VillageRuntimeLayoutSnapshot NextSnapshot;
	NextSnapshot.Revision = GetNextRuntimeLayoutRevision();
	NextSnapshot.RunSeed = LastRunSeed;
	NextSnapshot.bShouldLoad = bShouldLoad;
	NextSnapshot.Instances = Instances;
	RuntimeLayoutSnapshot = MoveTemp(NextSnapshot);
	ForceNetUpdate();

	UE_LOG(LogTemp, Log,
		TEXT("[VillageLayout] Published runtime snapshot Revision=%d Seed=%d Load=%s Instances=%d."),
		RuntimeLayoutSnapshot.Revision,
		RuntimeLayoutSnapshot.RunSeed,
		RuntimeLayoutSnapshot.bShouldLoad ? TEXT("true") : TEXT("false"),
		RuntimeLayoutSnapshot.Instances.Num());
}

void AGP_VillageLayoutDirector::PublishEmptyRuntimeLayoutSnapshot()
{
	if (RuntimeLayoutSnapshot.Revision > 0
		&& !RuntimeLayoutSnapshot.bShouldLoad
		&& RuntimeLayoutSnapshot.Instances.IsEmpty())
	{
		return;
	}

	PublishRuntimeLayoutSnapshot({}, false);
}

void AGP_VillageLayoutDirector::TryApplyReplicatedRuntimeLayout()
{
	if (HasAuthority()
		|| !HasActorBegunPlay()
		|| !GetWorld()
		|| !GetWorld()->IsGameWorld()
		|| RuntimeLayoutSnapshot.Revision <= 0
		|| RuntimeLayoutSnapshot.Revision == AppliedRuntimeLayoutRevision)
	{
		return;
	}

	AppliedRuntimeLayoutRevision = RuntimeLayoutSnapshot.Revision;
	UnloadActiveVillageLevels();
	SelectedSlotIds.Reset();
	LastRunSeed = RuntimeLayoutSnapshot.RunSeed;

	if (!RuntimeLayoutSnapshot.bShouldLoad)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[VillageLayout] Client applied unload snapshot Revision=%d Seed=%d."),
			RuntimeLayoutSnapshot.Revision,
			RuntimeLayoutSnapshot.RunSeed);
		return;
	}

	if (LastRunSeed < 0)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[VillageLayout] Client rejected runtime snapshot Revision=%d because RunSeed is invalid."),
			RuntimeLayoutSnapshot.Revision);
		return;
	}

	for (const FGP_VillageRuntimeInstance& Instance : RuntimeLayoutSnapshot.Instances)
	{
		SelectedSlotIds.Add(Instance.SlotId);
	}

	UE_LOG(LogTemp, Log,
		TEXT("[VillageLayout] Client applying runtime snapshot Revision=%d Seed=%d Instances=%d."),
		RuntimeLayoutSnapshot.Revision,
		LastRunSeed,
		RuntimeLayoutSnapshot.Instances.Num());
	if (!StreamRuntimeVillageLevels(RuntimeLayoutSnapshot.Instances))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[VillageLayout] Client failed to schedule runtime snapshot Revision=%d."),
			RuntimeLayoutSnapshot.Revision);
	}
}

void AGP_VillageLayoutDirector::OnRep_RuntimeLayoutSnapshot()
{
	TryApplyReplicatedRuntimeLayout();
}

void AGP_VillageLayoutDirector::UnloadActiveVillageLevels()
{
	ClearVillageStreamingTimeout();

	for (const TPair<FName, TObjectPtr<ULevelStreamingDynamic>>& Pair : ActiveVillageLevels)
	{
		ULevelStreamingDynamic* StreamingLevel = Pair.Value;
		if (!StreamingLevel)
		{
			continue;
		}

		if (ULevel* LoadedLevel = StreamingLevel->GetLoadedLevel())
		{
			for (AActor* Actor : LoadedLevel->Actors)
			{
				if (!IsValid(Actor))
				{
					continue;
				}

				TInlineComponentArray<UPCGComponent*> PCGComponents(Actor);
				for (UPCGComponent* PCGComponent : PCGComponents)
				{
					if (IsValid(PCGComponent))
					{
						PCGComponent->OnPCGGraphGeneratedDelegate.RemoveAll(this);
						PCGComponent->OnPCGGraphCancelledDelegate.RemoveAll(this);
						PCGComponent->CancelGeneration();
						PCGComponent->CleanupLocalImmediate(true);
					}
				}
			}
		}

		StreamingLevel->OnLevelLoaded.RemoveDynamic(this, &AGP_VillageLayoutDirector::HandleVillageLevelLoaded);
		StreamingLevel->OnLevelShown.RemoveDynamic(this, &AGP_VillageLayoutDirector::HandleVillageLevelShown);
		StreamingLevel->SetShouldBeVisible(false);
		StreamingLevel->SetShouldBeLoaded(false);
		StreamingLevel->SetIsRequestingUnloadAndRemoval(true);
	}

	ActiveVillageLevels.Reset();
	ConfiguredVillageLevelIds.Reset();
	GenerationRequestedVillageLevelIds.Reset();
	GeneratedVillageLevelIds.Reset();
	PendingVillagePCGComponents.Reset();
	ActiveVillagePCGSeeds.Reset();
	ActiveVillageInstanceNames.Reset();
	ExpectedVillageLevelCount = 0;
	bRuntimeLayoutReady = false;
}

void AGP_VillageLayoutDirector::ConfigureVillageGameplayActors(
	ULevelStreamingDynamic* StreamingLevel,
	FName SlotId) const
{
	ULevel* LoadedLevel = StreamingLevel ? StreamingLevel->GetLoadedLevel() : nullptr;
	if (!LoadedLevel)
	{
		return;
	}

	const AGP_VillageSlot* Slot = nullptr;
	for (const AGP_VillageSlot* Candidate : CachedSlots)
	{
		if (IsValid(Candidate) && Candidate->GetSlotId() == SlotId)
		{
			Slot = Candidate;
			break;
		}
	}
	if (!Slot)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[VillageLayout] Cannot configure gameplay actors for unknown slot '%s'."),
			*SlotId.ToString());
		return;
	}

	const EGPZoneStage Stage = ResolveZoneStage(Slot->GetGroupId(), SlotId);
	int32 ZoneCount = 0;
	for (AActor* Actor : LoadedLevel->Actors)
	{
		AGP_EnemySpawnVolume* Zone = Cast<AGP_EnemySpawnVolume>(Actor);
		if (!IsValid(Zone))
		{
			continue;
		}

		++ZoneCount;
		const FName RuntimeZoneId = ZoneCount == 1
			? SlotId
			: FName(*FString::Printf(TEXT("%s_%02d"), *SlotId.ToString(), ZoneCount));
		Zone->ConfigureRuntimeZone(
			RuntimeZoneId,
			Stage,
			GetZoneStageOrder(Stage),
			Slot->GetRegionId());
	}

	if (ZoneCount == 0)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[VillageLayout] Slot '%s' preset has no AGP_EnemySpawnVolume yet."),
			*SlotId.ToString());
		return;
	}

	UE_LOG(LogTemp, Log,
		TEXT("[VillageLayout] Configured %d gameplay zone(s) for slot '%s' Stage=%d RegionId=%d."),
		ZoneCount,
		*SlotId.ToString(),
		static_cast<int32>(Stage),
		Slot->GetRegionId());
}

bool AGP_VillageLayoutDirector::ConfigureVillagePCG(
	ULevelStreamingDynamic* StreamingLevel,
	FName SlotId,
	int32& OutPCGComponentCount,
	int32& OutRoadActorCount,
	int32& OutDistrictActorCount) const
{
	OutPCGComponentCount = 0;
	OutRoadActorCount = 0;
	OutDistrictActorCount = 0;

	ULevel* LoadedLevel = StreamingLevel ? StreamingLevel->GetLoadedLevel() : nullptr;
	if (!LoadedLevel)
	{
		return false;
	}

	TArray<UPCGComponent*> CityPCGComponents;
	TSet<FName> TemplateRoadTags;
	TSet<FName> TemplateDistrictTags;
	for (AActor* Actor : LoadedLevel->Actors)
	{
		if (!IsValid(Actor))
		{
			continue;
		}

		TInlineComponentArray<UPCGComponent*> PCGComponents(Actor);
		for (UPCGComponent* PCGComponent : PCGComponents)
		{
			FName TemplateRoadTag;
			FName TemplateDistrictTag;
			if (GetCityPCGTemplateTags(PCGComponent, TemplateRoadTag, TemplateDistrictTag))
			{
				CityPCGComponents.Add(PCGComponent);
				TemplateRoadTags.Add(TemplateRoadTag);
				TemplateDistrictTags.Add(TemplateDistrictTag);
			}
		}
	}

	if (CityPCGComponents.Num() != 1)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[VillageLayout] Slot '%s' preset must contain exactly one city PCG component, but found %d."),
			*SlotId.ToString(),
			CityPCGComponents.Num());
		return false;
	}

	const FName UniqueRoadTag = MakeVillageRoleTag(SlotId, TEXT("Road"));
	const FName UniqueDistrictTag = MakeVillageRoleTag(SlotId, TEXT("District"));
	for (AActor* Actor : LoadedLevel->Actors)
	{
		if (!IsValid(Actor))
		{
			continue;
		}

		const bool bIsRoadActor = Actor->Tags.ContainsByPredicate(
			[&TemplateRoadTags](FName Tag) { return TemplateRoadTags.Contains(Tag); });
		const bool bIsDistrictActor = Actor->Tags.ContainsByPredicate(
			[&TemplateDistrictTags](FName Tag) { return TemplateDistrictTags.Contains(Tag); });
		if (bIsRoadActor)
		{
			Actor->Tags.AddUnique(UniqueRoadTag);
			for (FName TemplateRoadTag : TemplateRoadTags)
			{
				Actor->Tags.Remove(TemplateRoadTag);
			}
			++OutRoadActorCount;
		}
		if (bIsDistrictActor)
		{
			Actor->Tags.AddUnique(UniqueDistrictTag);
			for (FName TemplateDistrictTag : TemplateDistrictTags)
			{
				Actor->Tags.Remove(TemplateDistrictTag);
			}
			++OutDistrictActorCount;
		}
	}

	if (OutRoadActorCount == 0 || OutDistrictActorCount == 0)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[VillageLayout] Slot '%s' preset is missing tagged source actors (Road=%d District=%d)."),
			*SlotId.ToString(),
			OutRoadActorCount,
			OutDistrictActorCount);
		return false;
	}

	const int32* PCGSeed = ActiveVillagePCGSeeds.Find(SlotId);
	if (!PCGSeed)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[VillageLayout] Slot '%s' has no runtime PCG seed."),
			*SlotId.ToString());
		return false;
	}

	for (UPCGComponent* PCGComponent : CityPCGComponents)
	{
		UPCGGraphInstance* GraphInstance = IsValid(PCGComponent) ? PCGComponent->GetGraphInstance() : nullptr;
		if (!GraphInstance
			|| GraphInstance->SetGraphParameter<FName>(TEXT("RoadTag"), UniqueRoadTag) != EPropertyBagResult::Success
			|| GraphInstance->SetGraphParameter<FName>(TEXT("DistrictTag"), UniqueDistrictTag) != EPropertyBagResult::Success)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[VillageLayout] Failed to override PCG tags for slot '%s'."),
				*SlotId.ToString());
			return false;
		}

		PCGComponent->Seed = *PCGSeed;
		PCGComponent->bActivated = true;
		PCGComponent->bIsComponentPartitioned = false;
		PCGComponent->GenerationTrigger = EPCGComponentGenerationTrigger::GenerateOnDemand;
		++OutPCGComponentCount;
	}

	return true;
}

bool AGP_VillageLayoutDirector::GenerateVillagePCG(
	ULevelStreamingDynamic* StreamingLevel,
	FName SlotId,
	int32& OutPCGComponentCount)
{
	OutPCGComponentCount = 0;
	ULevel* LoadedLevel = StreamingLevel ? StreamingLevel->GetLoadedLevel() : nullptr;
	if (!LoadedLevel)
	{
		return false;
	}

	TArray<UPCGComponent*> CityPCGComponents;
	for (AActor* Actor : LoadedLevel->Actors)
	{
		if (!IsValid(Actor))
		{
			continue;
		}

		TInlineComponentArray<UPCGComponent*> PCGComponents(Actor);
		for (UPCGComponent* PCGComponent : PCGComponents)
		{
			FName RoadTag;
			FName DistrictTag;
			if (GetCityPCGTemplateTags(PCGComponent, RoadTag, DistrictTag))
			{
				CityPCGComponents.Add(PCGComponent);
			}
		}
	}

	if (CityPCGComponents.Num() != 1)
	{
		return false;
	}

	UPCGComponent* PCGComponent = CityPCGComponents[0];
	PCGComponent->OnPCGGraphGeneratedDelegate.RemoveAll(this);
	PCGComponent->OnPCGGraphCancelledDelegate.RemoveAll(this);
	PCGComponent->OnPCGGraphGeneratedDelegate.AddUObject(
		this,
		&AGP_VillageLayoutDirector::HandleVillagePCGGenerated);
	PCGComponent->OnPCGGraphCancelledDelegate.AddUObject(
		this,
		&AGP_VillageLayoutDirector::HandleVillagePCGCancelled);
	PendingVillagePCGComponents.Add(PCGComponent, SlotId);

	const FPCGTaskId TaskId = PCGComponent->GenerateLocalGetTaskId(true);
	const bool bCompletedSynchronously = GeneratedVillageLevelIds.Contains(SlotId);
	if ((TaskId == InvalidPCGTaskId && !bCompletedSynchronously) || bVillageLevelBatchFailed)
	{
		PendingVillagePCGComponents.Remove(PCGComponent);
		PCGComponent->OnPCGGraphGeneratedDelegate.RemoveAll(this);
		PCGComponent->OnPCGGraphCancelledDelegate.RemoveAll(this);
		return false;
	}

	OutPCGComponentCount = 1;
	return true;
}

bool AGP_VillageLayoutDirector::RequestNextVillagePCGGeneration()
{
	if (bVillageLevelBatchFailed || ExpectedVillageLevelCount <= 0)
	{
		return false;
	}

	// Keep exactly one city graph in flight so concurrent execution cannot make the
	// output of two streamed copies interfere with each other.
	if (PendingVillagePCGComponents.Num() > 0)
	{
		return true;
	}

	TGuardValue<bool> SchedulingGuard(bSchedulingVillagePCG, true);
	for (FName SlotId : SelectedSlotIds)
	{
		if (GenerationRequestedVillageLevelIds.Contains(SlotId))
		{
			continue;
		}

		TObjectPtr<ULevelStreamingDynamic>* StreamingLevel = ActiveVillageLevels.Find(SlotId);
		if (!StreamingLevel || !*StreamingLevel)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[VillageLayout] Cannot generate slot '%s' because its streaming level is unavailable."),
				*SlotId.ToString());
			return false;
		}

		// Add before GenerateLocalGetTaskId because cancellation/completion delegates may
		// execute synchronously.
		GenerationRequestedVillageLevelIds.Add(SlotId);
		int32 PCGComponentCount = 0;
		if (!GenerateVillagePCG(*StreamingLevel, SlotId, PCGComponentCount))
		{
			GenerationRequestedVillageLevelIds.Remove(SlotId);
			UE_LOG(LogTemp, Error,
				TEXT("[VillageLayout] Slot '%s' failed to schedule city PCG."),
				*SlotId.ToString());
			return false;
		}

		UE_LOG(LogTemp, Log,
			TEXT("[VillageLayout] Requested sequential generation for slot '%s' on %d PCG component(s); RoadTag='%s' DistrictTag='%s'."),
			*SlotId.ToString(),
			PCGComponentCount,
			*MakeVillageRoleTag(SlotId, TEXT("Road")).ToString(),
			*MakeVillageRoleTag(SlotId, TEXT("District")).ToString());

		if (PendingVillagePCGComponents.Num() > 0)
		{
			return true;
		}

		// A graph can complete synchronously. Continue to the next selected slot only
		// after that completion has been recorded.
		if (!GeneratedVillageLevelIds.Contains(SlotId))
		{
			return false;
		}
	}

	return GeneratedVillageLevelIds.Num() == ExpectedVillageLevelCount;
}

void AGP_VillageLayoutDirector::LogVillagePCGResult(
	FName SlotId,
	UPCGComponent* PCGComponent) const
{
	const TObjectPtr<ULevelStreamingDynamic>* StreamingLevelPtr = ActiveVillageLevels.Find(SlotId);
	const ULevelStreamingDynamic* StreamingLevel = StreamingLevelPtr ? StreamingLevelPtr->Get() : nullptr;
	ULevel* LoadedLevel = StreamingLevel ? StreamingLevel->GetLoadedLevel() : nullptr;

	int32 ValidActorCount = 0;
	int32 StaticMeshComponentCount = 0;
	int32 ISMComponentCount = 0;
	int32 ISMInstanceCount = 0;
	FBox MeshBounds(ForceInit);
	if (LoadedLevel)
	{
		for (AActor* Actor : LoadedLevel->Actors)
		{
			if (!IsValid(Actor))
			{
				continue;
			}

			++ValidActorCount;
			TInlineComponentArray<UStaticMeshComponent*> MeshComponents(Actor);
			for (UStaticMeshComponent* MeshComponent : MeshComponents)
			{
				if (!IsValid(MeshComponent) || !MeshComponent->GetStaticMesh())
				{
					continue;
				}

				++StaticMeshComponentCount;
				MeshBounds += MeshComponent->Bounds.GetBox();
				if (const UInstancedStaticMeshComponent* ISMComponent =
					Cast<UInstancedStaticMeshComponent>(MeshComponent))
				{
					++ISMComponentCount;
					ISMInstanceCount += ISMComponent->GetInstanceCount();
				}
			}
		}
	}

	int32 ManagedResourceCount = 0;
	int32 ManagedISMComponentCount = 0;
	int32 ManagedISMInstanceCount = 0;
	int32 ManagedActorCount = 0;
	if (IsValid(PCGComponent))
	{
		PCGComponent->ForEachManagedResource(
			[&](UPCGManagedResource* ManagedResource)
			{
				++ManagedResourceCount;
				if (const UPCGManagedISMComponent* ManagedISM = Cast<UPCGManagedISMComponent>(ManagedResource))
				{
					if (const UInstancedStaticMeshComponent* ISMComponent = ManagedISM->GetComponent())
					{
						++ManagedISMComponentCount;
						ManagedISMInstanceCount += ISMComponent->GetInstanceCount();
					}
				}
				else if (const UPCGManagedActors* ManagedActors = Cast<UPCGManagedActors>(ManagedResource))
				{
					ManagedActorCount += ManagedActors->GetConstGeneratedActors().Num();
				}
			});
	}

	const FVector OwnerLocation = IsValid(PCGComponent) && IsValid(PCGComponent->GetOwner())
		? PCGComponent->GetOwner()->GetActorLocation()
		: FVector::ZeroVector;
	const FVector LevelLocation = StreamingLevel
		? StreamingLevel->LevelTransform.GetLocation()
		: FVector::ZeroVector;
	const int32 OutputDataCount = IsValid(PCGComponent)
		? PCGComponent->GetGeneratedGraphOutput().TaggedData.Num()
		: 0;
	const FString LevelPackage = LoadedLevel && LoadedLevel->GetOutermost()
		? LoadedLevel->GetOutermost()->GetName()
		: TEXT("None");
	const FString BoundsText = MeshBounds.IsValid
		? MeshBounds.ToString()
		: TEXT("Invalid");

	UE_LOG(LogTemp, Log,
		TEXT("[VillageLayout] Result slot '%s': Level='%s' LevelLocation=%s OwnerLocation=%s OutputData=%d ManagedResources=%d ManagedISMComponents=%d ManagedISMInstances=%d ManagedActors=%d LevelActors=%d LevelMeshComponents=%d LevelISMComponents=%d LevelISMInstances=%d LevelMeshBounds=%s."),
		*SlotId.ToString(),
		*LevelPackage,
		*LevelLocation.ToCompactString(),
		*OwnerLocation.ToCompactString(),
		OutputDataCount,
		ManagedResourceCount,
		ManagedISMComponentCount,
		ManagedISMInstanceCount,
		ManagedActorCount,
		ValidActorCount,
		StaticMeshComponentCount,
		ISMComponentCount,
		ISMInstanceCount,
		*BoundsText);
}

bool AGP_VillageLayoutDirector::GetCityPCGTemplateTags(
	const UPCGComponent* PCGComponent,
	FName& OutRoadTag,
	FName& OutDistrictTag) const
{
	OutRoadTag = NAME_None;
	OutDistrictTag = NAME_None;

	const UPCGGraph* Graph = IsValid(PCGComponent) ? PCGComponent->GetGraph() : nullptr;
	const UPCGGraphInstance* GraphInstance = IsValid(PCGComponent) ? PCGComponent->GetGraphInstance() : nullptr;
	if (!Graph
		|| Graph->GetPathName() != CityPCGGraphObjectPath
		|| !GraphInstance)
	{
		return false;
	}

	const TValueOrError<FName, EPropertyBagResult> RoadTagResult =
		GraphInstance->GetGraphParameter<FName>(TEXT("RoadTag"));
	const TValueOrError<FName, EPropertyBagResult> DistrictTagResult =
		GraphInstance->GetGraphParameter<FName>(TEXT("DistrictTag"));
	if (RoadTagResult.HasError() || DistrictTagResult.HasError())
	{
		return false;
	}

	OutRoadTag = RoadTagResult.GetValue();
	OutDistrictTag = DistrictTagResult.GetValue();
	return !OutRoadTag.IsNone() && !OutDistrictTag.IsNone();
}

int32 AGP_VillageLayoutDirector::MakeVillagePCGSeed(FName SlotId) const
{
	const uint32 SlotHash = FCrc::StrCrc32(*SlotId.ToString());
	return static_cast<int32>(HashCombineFast(static_cast<uint32>(LastRunSeed), SlotHash) & MAX_int32);
}

int32 AGP_VillageLayoutDirector::GetNextRuntimeLayoutRevision() const
{
	return RuntimeLayoutSnapshot.Revision >= MAX_int32
		? 1
		: RuntimeLayoutSnapshot.Revision + 1;
}

FString AGP_VillageLayoutDirector::MakeStreamingLevelName(FName SlotId) const
{
	return FString::Printf(
		TEXT("Village_%08X_%08X"),
		static_cast<uint32>(LastRunSeed),
		FCrc::StrCrc32(*SlotId.ToString()));
}

FName AGP_VillageLayoutDirector::MakeVillageRoleTag(FName SlotId, const TCHAR* RoleSuffix) const
{
	const FName* ActiveInstanceName = ActiveVillageInstanceNames.Find(SlotId);
	const FString Prefix = ActiveInstanceName
		? ActiveInstanceName->ToString()
		: MakeStreamingLevelName(SlotId);
	return FName(*FString::Printf(TEXT("%s_%s"), *Prefix, RoleSuffix));
}

void AGP_VillageLayoutDirector::HandleVillageLevelLoaded()
{
	if (ExpectedVillageLevelCount <= 0
		|| ActiveVillageLevels.Num() != ExpectedVillageLevelCount)
	{
		return;
	}

	for (const TPair<FName, TObjectPtr<ULevelStreamingDynamic>>& Pair : ActiveVillageLevels)
	{
		if (!Pair.Value || !Pair.Value->IsLevelLoaded())
		{
			return;
		}
	}

	bool bConfigurationFailed = false;
	for (const TPair<FName, TObjectPtr<ULevelStreamingDynamic>>& Pair : ActiveVillageLevels)
	{
		if (ConfiguredVillageLevelIds.Contains(Pair.Key) || !Pair.Value || !Pair.Value->GetLoadedLevel())
		{
			continue;
		}

		int32 PCGComponentCount = 0;
		int32 RoadActorCount = 0;
		int32 DistrictActorCount = 0;
		if (HasAuthority())
		{
			ConfigureVillageGameplayActors(Pair.Value, Pair.Key);
		}
		if (!ConfigureVillagePCG(
			Pair.Value,
			Pair.Key,
			PCGComponentCount,
			RoadActorCount,
			DistrictActorCount))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[VillageLayout] Slot '%s' failed PCG isolation; rolling back the village batch."),
				*Pair.Key.ToString());
			bConfigurationFailed = true;
			break;
		}

		ConfiguredVillageLevelIds.Add(Pair.Key);
		UE_LOG(LogTemp, Log,
			TEXT("[VillageLayout] Loaded slot '%s'; tagged Road=%d District=%d and configured %d city PCG component(s)."),
			*Pair.Key.ToString(),
			RoadActorCount,
			DistrictActorCount,
			PCGComponentCount);
	}

	if (bConfigurationFailed)
	{
		bVillageLevelBatchFailed = true;
		LastSelectionSummary += TEXT(" Runtime=ConfigurationFailed");
		PublishEmptyRuntimeLayoutSnapshot();
		UnloadActiveVillageLevels();
		return;
	}

	if (ExpectedVillageLevelCount > 0
		&& ActiveVillageLevels.Num() == ExpectedVillageLevelCount
		&& ConfiguredVillageLevelIds.Num() == ExpectedVillageLevelCount)
	{
		for (const TPair<FName, TObjectPtr<ULevelStreamingDynamic>>& Pair : ActiveVillageLevels)
		{
			if (Pair.Value)
			{
				Pair.Value->SetShouldBeVisible(true);
			}
		}
	}
}

void AGP_VillageLayoutDirector::HandleVillageLevelShown()
{
	if (ExpectedVillageLevelCount <= 0
		|| ActiveVillageLevels.Num() != ExpectedVillageLevelCount
		|| ConfiguredVillageLevelIds.Num() != ExpectedVillageLevelCount)
	{
		return;
	}

	for (const TPair<FName, TObjectPtr<ULevelStreamingDynamic>>& Pair : ActiveVillageLevels)
	{
		if (!Pair.Value || !Pair.Value->IsLevelVisible())
		{
			return;
		}
	}

	if (!RequestNextVillagePCGGeneration())
	{
		bVillageLevelBatchFailed = true;
		LastSelectionSummary += TEXT(" Runtime=GenerationFailed");
		UE_LOG(LogTemp, Error,
			TEXT("[VillageLayout] Failed to start sequential city PCG generation; rolling back the village batch."));
		PublishEmptyRuntimeLayoutSnapshot();
		UnloadActiveVillageLevels();
	}
}

void AGP_VillageLayoutDirector::HandleVillagePCGGenerated(UPCGComponent* PCGComponent)
{
	FName SlotId;
	if (!PendingVillagePCGComponents.RemoveAndCopyValue(PCGComponent, SlotId))
	{
		return;
	}

	if (IsValid(PCGComponent))
	{
		PCGComponent->OnPCGGraphGeneratedDelegate.RemoveAll(this);
		PCGComponent->OnPCGGraphCancelledDelegate.RemoveAll(this);
	}

	GeneratedVillageLevelIds.Add(SlotId);
	LogVillagePCGResult(SlotId, PCGComponent);
	UE_LOG(LogTemp, Log,
		TEXT("[VillageLayout] Completed city PCG generation for slot '%s' (%d/%d)."),
		*SlotId.ToString(),
		GeneratedVillageLevelIds.Num(),
		ExpectedVillageLevelCount);

	if (ExpectedVillageLevelCount > 0
		&& GeneratedVillageLevelIds.Num() == ExpectedVillageLevelCount)
	{
		ClearVillageStreamingTimeout();
		MarkRuntimeLayoutReady();
		return;
	}

	if (!bSchedulingVillagePCG && !RequestNextVillagePCGGeneration())
	{
		bVillageLevelBatchFailed = true;
		LastSelectionSummary += TEXT(" Runtime=GenerationFailed");
		UE_LOG(LogTemp, Error,
			TEXT("[VillageLayout] Failed to continue sequential city PCG generation after slot '%s'; rolling back the village batch."),
			*SlotId.ToString());
		PublishEmptyRuntimeLayoutSnapshot();
		UnloadActiveVillageLevels();
	}
}

void AGP_VillageLayoutDirector::HandleVillagePCGCancelled(UPCGComponent* PCGComponent)
{
	FName SlotId;
	if (!PendingVillagePCGComponents.RemoveAndCopyValue(PCGComponent, SlotId))
	{
		return;
	}

	if (IsValid(PCGComponent))
	{
		PCGComponent->OnPCGGraphGeneratedDelegate.RemoveAll(this);
		PCGComponent->OnPCGGraphCancelledDelegate.RemoveAll(this);
	}

	bVillageLevelBatchFailed = true;
	LastSelectionSummary += TEXT(" Runtime=GenerationCancelled");
	UE_LOG(LogTemp, Error,
		TEXT("[VillageLayout] City PCG generation was cancelled for slot '%s'; rolling back the village batch."),
		*SlotId.ToString());
	PublishEmptyRuntimeLayoutSnapshot();
	if (!bSchedulingVillagePCG)
	{
		UnloadActiveVillageLevels();
	}
}

void AGP_VillageLayoutDirector::HandleVillageStreamingTimeout()
{
	if (ExpectedVillageLevelCount <= 0
		|| GeneratedVillageLevelIds.Num() == ExpectedVillageLevelCount)
	{
		ClearVillageStreamingTimeout();
		return;
	}

	bVillageLevelBatchFailed = true;
	LastSelectionSummary += TEXT(" Runtime=Timeout");
	UE_LOG(LogTemp, Error,
		TEXT("[VillageLayout] Village batch timed out after %.1f seconds (Configured=%d Requested=%d Completed=%d Expected=%d)."),
		VillageStreamingTimeoutSeconds,
		ConfiguredVillageLevelIds.Num(),
		GenerationRequestedVillageLevelIds.Num(),
		GeneratedVillageLevelIds.Num(),
		ExpectedVillageLevelCount);
	PublishEmptyRuntimeLayoutSnapshot();
	UnloadActiveVillageLevels();
}

void AGP_VillageLayoutDirector::ClearVillageStreamingTimeout()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(VillageStreamingTimeoutHandle);
	}
	else
	{
		VillageStreamingTimeoutHandle.Invalidate();
	}
}

void AGP_VillageLayoutDirector::MarkRuntimeLayoutReady()
{
	if (bRuntimeLayoutReady || !GetWorld() || !GetWorld()->IsGameWorld())
	{
		return;
	}

	bRuntimeLayoutReady = true;
	UE_LOG(LogTemp, Log,
		TEXT("[VillageLayout] Runtime layout ready. Selected=%d Generated=%d."),
		SelectedSlotIds.Num(),
		GeneratedVillageLevelIds.Num());
	OnRuntimeLayoutReady.Broadcast();

	if (!HasAuthority())
	{
		ClientVisualReadyRetryAttempts = 0;
		TryNotifyClientVisualReady();
	}
}

void AGP_VillageLayoutDirector::TryNotifyClientVisualReady()
{
	constexpr int32 MaxAttempts = 40;
	constexpr float RetryIntervalSeconds = 0.25f;

	UWorld* World = GetWorld();
	if (!World || HasAuthority() || !bRuntimeLayoutReady)
	{
		return;
	}

	++ClientVisualReadyRetryAttempts;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		AGP_PlayerController* PlayerController = Cast<AGP_PlayerController>(It->Get());
		if (IsValid(PlayerController) && PlayerController->IsLocalController())
		{
			World->GetTimerManager().ClearTimer(ClientVisualReadyRetryHandle);
			PlayerController->NotifyVillageVisualReady(AppliedRuntimeLayoutRevision);
			UE_LOG(LogTemp, Log,
				TEXT("[VillageLayout] Sent client visual-ready ACK for revision %d after %d attempt(s)."),
				AppliedRuntimeLayoutRevision,
				ClientVisualReadyRetryAttempts);
			return;
		}
	}

	if (ClientVisualReadyRetryAttempts >= MaxAttempts)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[VillageLayout] No local GP_PlayerController could send visual-ready ACK for revision %d after %d attempts."),
			AppliedRuntimeLayoutRevision,
			ClientVisualReadyRetryAttempts);
		return;
	}

	World->GetTimerManager().SetTimer(
		ClientVisualReadyRetryHandle,
		this,
		&ThisClass::TryNotifyClientVisualReady,
		RetryIntervalSeconds,
		false);
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
