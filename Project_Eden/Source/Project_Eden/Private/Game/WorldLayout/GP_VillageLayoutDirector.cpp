#include "Game/WorldLayout/GP_VillageLayoutDirector.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Level.h"
#include "Engine/LevelStreamingDynamic.h"
#include "EngineUtils.h"
#include "Game/GP_GameState.h"
#include "Game/WorldLayout/GP_VillageSelectionPolicy.h"
#include "Game/WorldLayout/GP_VillageSlot.h"
#include "Misc/Crc.h"
#include "PCGComponent.h"
#include "WorldPartition/DataLayer/DataLayerAsset.h"
#include "WorldPartition/DataLayer/DataLayerManager.h"

namespace
{
	struct FGPVillageDataLayerBinding
	{
		const AGP_VillageSlot* Slot = nullptr;
		const UDataLayerAsset* Asset = nullptr;
		const UDataLayerInstance* Instance = nullptr;
	};
}

AGP_VillageLayoutDirector::AGP_VillageLayoutDirector()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	FGP_VillageGroupRule DefaultRule;
	GroupRules.Add(DefaultRule);

	VillageLevelPreset = TSoftObjectPtr<UWorld>(
		FSoftObjectPath(TEXT("/Game/WorldLayout/L_Village_00.L_Village_00")));

#if WITH_EDITORONLY_DATA
	bIsSpatiallyLoaded = false;
#endif
}

void AGP_VillageLayoutDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnloadActiveVillageLevels();
	Super::EndPlay(EndPlayReason);
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

	const bool bIsGameWorld = GetWorld() && GetWorld()->IsGameWorld();
	const bool bShouldApplyDataLayers = bIsGameWorld && bApplyDataLayersAtRuntime;
	const bool bDataLayersApplied = !bShouldApplyDataLayers || ApplyDataLayerStates(SelectedSet);
	const bool bShouldStreamVillageLevel = bIsGameWorld && bStreamVillageLevelAtRuntime;
	const bool bVillageLevelScheduled = !bShouldStreamVillageLevel
		|| (Result.bSucceeded && StreamSelectedVillageLevel(SelectedSet));

	TArray<FString> SelectedNames;
	for (FName SlotId : SelectedSlotIds)
	{
		SelectedNames.Add(SlotId.ToString());
	}

	const TCHAR* DataLayerStatus = !bIsGameWorld
		? TEXT("Preview")
		: (!bApplyDataLayersAtRuntime ? TEXT("Disabled") : (bDataLayersApplied ? TEXT("Applied") : TEXT("Failed")));
	const TCHAR* LevelInstanceStatus = !bIsGameWorld
		? TEXT("Preview")
		: (!bStreamVillageLevelAtRuntime ? TEXT("Disabled") : (bVillageLevelScheduled ? TEXT("Scheduled") : TEXT("Failed")));
	LastSelectionSummary = FString::Printf(
		TEXT("Seed=%d Candidates=%d Selected=[%s] DataLayers=%s LevelInstance=%s"),
		InRunSeed,
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

	return Result.bSucceeded && bDataLayersApplied && bVillageLevelScheduled;
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

bool AGP_VillageLayoutDirector::StreamSelectedVillageLevel(const TSet<FName>& InSelectedSlotIds)
{
	UnloadActiveVillageLevels();

	if (InSelectedSlotIds.IsEmpty())
	{
		return true;
	}

	if (InSelectedSlotIds.Num() > 1)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[VillageLayout] The single-preset V1 supports one active village, but %d slots were selected."),
			InSelectedSlotIds.Num());
		return false;
	}

	if (VillageLevelPreset.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("[VillageLayout] VillageLevelPreset is not assigned."));
		return false;
	}

	const FName SlotId = *InSelectedSlotIds.CreateConstIterator();
	const TObjectPtr<AGP_VillageSlot>* SlotPtr = CachedSlots.FindByPredicate(
		[SlotId](const TObjectPtr<AGP_VillageSlot>& Slot)
		{
			return IsValid(Slot) && Slot->GetSlotId() == SlotId;
		});
	const AGP_VillageSlot* Slot = SlotPtr ? SlotPtr->Get() : nullptr;
	if (!Slot)
	{
		UE_LOG(LogTemp, Error, TEXT("[VillageLayout] Selected slot '%s' is no longer available."), *SlotId.ToString());
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const FTransform SpawnTransform(Slot->GetActorRotation(), Slot->GetActorLocation(), FVector::OneVector);
	const FString StableLevelName = MakeStreamingLevelName(SlotId);
	ULevelStreamingDynamic::FLoadLevelInstanceParams Params(
		World,
		VillageLevelPreset.ToSoftObjectPath().GetLongPackageName(),
		SpawnTransform);
	Params.OptionalLevelNameOverride = &StableLevelName;
	Params.bInitiallyVisible = false;
	Params.LevelStreamingCreatedCallback = [this](ULevelStreaming* CreatedStreamingLevel)
	{
		if (CreatedStreamingLevel)
		{
			CreatedStreamingLevel->OnLevelLoaded.AddUniqueDynamic(this, &AGP_VillageLayoutDirector::HandleVillageLevelLoaded);
			CreatedStreamingLevel->OnLevelShown.AddUniqueDynamic(this, &AGP_VillageLayoutDirector::HandleVillageLevelShown);
		}
	};

	bool bLoadScheduled = false;
	ULevelStreamingDynamic* StreamingLevel = ULevelStreamingDynamic::LoadLevelInstance(Params, bLoadScheduled);
	if (!bLoadScheduled || !StreamingLevel)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[VillageLayout] Failed to schedule village preset '%s' for slot '%s'."),
			*VillageLevelPreset.ToString(),
			*SlotId.ToString());
		return false;
	}

	ActiveVillageLevels.Add(SlotId, StreamingLevel);
	UE_LOG(LogTemp, Log,
		TEXT("[VillageLayout] Scheduled village preset '%s' at slot '%s' Location=%s."),
		*VillageLevelPreset.ToString(),
		*SlotId.ToString(),
		*Slot->GetActorLocation().ToCompactString());

	// Covers editor/synchronous loading where the delegate may run before the map entry is stored.
	HandleVillageLevelLoaded();
	HandleVillageLevelShown();
	return true;
}

void AGP_VillageLayoutDirector::UnloadActiveVillageLevels()
{
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
	GeneratedVillageLevelIds.Reset();
}

int32 AGP_VillageLayoutDirector::ConfigureVillagePCG(ULevelStreamingDynamic* StreamingLevel, FName SlotId) const
{
	ULevel* LoadedLevel = StreamingLevel ? StreamingLevel->GetLoadedLevel() : nullptr;
	if (!LoadedLevel)
	{
		return 0;
	}

	const int32 PCGSeed = MakeVillagePCGSeed(SlotId);
	int32 ComponentCount = 0;
	for (AActor* Actor : LoadedLevel->Actors)
	{
		if (!IsValid(Actor))
		{
			continue;
		}

		TInlineComponentArray<UPCGComponent*> PCGComponents(Actor);
		for (UPCGComponent* PCGComponent : PCGComponents)
		{
			if (!IsValid(PCGComponent))
			{
				continue;
			}

			PCGComponent->Seed = PCGSeed;
			PCGComponent->bActivated = true;
			PCGComponent->bIsComponentPartitioned = false;
			PCGComponent->GenerationTrigger = EPCGComponentGenerationTrigger::GenerateOnDemand;
			++ComponentCount;
		}
	}

	return ComponentCount;
}

int32 AGP_VillageLayoutDirector::GenerateVillagePCG(ULevelStreamingDynamic* StreamingLevel) const
{
	ULevel* LoadedLevel = StreamingLevel ? StreamingLevel->GetLoadedLevel() : nullptr;
	if (!LoadedLevel)
	{
		return 0;
	}

	int32 ComponentCount = 0;
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
				PCGComponent->GenerateLocal(true);
				++ComponentCount;
			}
		}
	}

	return ComponentCount;
}

int32 AGP_VillageLayoutDirector::MakeVillagePCGSeed(FName SlotId) const
{
	const uint32 SlotHash = FCrc::StrCrc32(*SlotId.ToString());
	return static_cast<int32>(HashCombineFast(static_cast<uint32>(LastRunSeed), SlotHash) & MAX_int32);
}

FString AGP_VillageLayoutDirector::MakeStreamingLevelName(FName SlotId) const
{
	return FString::Printf(
		TEXT("Village_%08X_%08X"),
		static_cast<uint32>(LastRunSeed),
		FCrc::StrCrc32(*SlotId.ToString()));
}

void AGP_VillageLayoutDirector::HandleVillageLevelLoaded()
{
	for (const TPair<FName, TObjectPtr<ULevelStreamingDynamic>>& Pair : ActiveVillageLevels)
	{
		if (ConfiguredVillageLevelIds.Contains(Pair.Key) || !Pair.Value || !Pair.Value->GetLoadedLevel())
		{
			continue;
		}

		const int32 PCGComponentCount = ConfigureVillagePCG(Pair.Value, Pair.Key);
		ConfiguredVillageLevelIds.Add(Pair.Key);
		Pair.Value->SetShouldBeVisible(true);
		UE_LOG(LogTemp, Log,
			TEXT("[VillageLayout] Loaded slot '%s'; configured %d PCG component(s)."),
			*Pair.Key.ToString(),
			PCGComponentCount);
	}
}

void AGP_VillageLayoutDirector::HandleVillageLevelShown()
{
	for (const TPair<FName, TObjectPtr<ULevelStreamingDynamic>>& Pair : ActiveVillageLevels)
	{
		if (GeneratedVillageLevelIds.Contains(Pair.Key)
			|| !ConfiguredVillageLevelIds.Contains(Pair.Key)
			|| !Pair.Value
			|| !Pair.Value->IsLevelVisible())
		{
			continue;
		}

		const int32 PCGComponentCount = GenerateVillagePCG(Pair.Value);
		GeneratedVillageLevelIds.Add(Pair.Key);
		UE_LOG(LogTemp, Log,
			TEXT("[VillageLayout] Shown slot '%s'; requested generation on %d PCG component(s)."),
			*Pair.Key.ToString(),
			PCGComponentCount);
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
