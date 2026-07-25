#include "Game/WorldLayout/GP_VillageLayoutDirector.h"

#include "Engine/Level.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/World.h"
#include "Game/GP_EnemySpawnVolume.h"
#include "Game/WorldLayout/GP_VillageSlot.h"
#include "Misc/Crc.h"
#include "PCGComponent.h"
#include "Player/GP_PlayerController.h"
#include "WorldPartition/DataLayer/DataLayerAsset.h"
#include "WorldPartition/DataLayer/DataLayerInstance.h"
#include "WorldPartition/DataLayer/DataLayerManager.h"

namespace
{
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
