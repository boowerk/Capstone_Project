#include "Game/WorldLayout/GP_VillageLayoutDirector.h"

#include "Engine/Level.h"
#include "Engine/World.h"
#include "Game/WorldLayout/GP_VillageSlot.h"
#include "Misc/ScopeExit.h"
#include "PCGComponent.h"

#if WITH_EDITOR
#include "LevelInstance/LevelInstanceActor.h"
#include "LevelInstance/LevelInstanceLevelStreaming.h"
#include "LevelInstance/LevelInstanceSubsystem.h"
#endif

void AGP_VillageLayoutDirector::RebuildPreview()
{
#if WITH_EDITOR
	if (GetWorld() && !GetWorld()->IsGameWorld())
	{
		ClearEditorVillagePreview();
		if (BuildSelectionForSeed(PreviewSeed) && !BuildEditorVillagePreview())
		{
			UE_LOG(LogTemp, Error,
				TEXT("[VillageLayout] Failed to build the transient editor village preview."));
		}
		return;
	}
#endif

	BuildSelectionForSeed(PreviewSeed);
}

void AGP_VillageLayoutDirector::ClearVillagePreview()
{
	ClearEditorVillagePreview();
}

void AGP_VillageLayoutDirector::RefreshFootprintPreview()
{
#if WITH_EDITOR
	ClearEditorVillagePreview();
#endif

	CollectSlots();
	AssignVillagePresets(PreviewSeed);
	ApplyFootprintPreview();
}

void AGP_VillageLayoutDirector::RefreshFootprintPreviewIgnoringSlot(
	const AGP_VillageSlot* IgnoredSlot)
{
#if WITH_EDITOR
	ClearEditorVillagePreview();
#endif

	CollectSlots();
	CachedSlots.RemoveAll([IgnoredSlot](const AGP_VillageSlot* Slot)
	{
		return Slot == IgnoredSlot;
	});
	AssignVillagePresets(PreviewSeed);
	ApplyFootprintPreview();
}

bool AGP_VillageLayoutDirector::BuildEditorVillagePreview()
{
#if WITH_EDITOR
	UWorld* World = GetWorld();
	if (!World || World->IsGameWorld())
	{
		return false;
	}

	ULevelInstanceSubsystem* LevelInstanceSubsystem = World->GetSubsystem<ULevelInstanceSubsystem>();
	if (!LevelInstanceSubsystem)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[VillageLayout] Editor preview requires a Level Instance subsystem."));
		return false;
	}

	const bool bWasWorldPackageDirty = World->GetOutermost()->IsDirty();
	TMap<TWeakObjectPtr<UPackage>, bool> PreviewPackageDirtyStates;
	ON_SCOPE_EXIT
	{
		if (!bWasWorldPackageDirty && IsValid(World))
		{
			World->GetOutermost()->SetDirtyFlag(false);
		}
		for (const TPair<TWeakObjectPtr<UPackage>, bool>& Pair : PreviewPackageDirtyStates)
		{
			if (!Pair.Value && Pair.Key.IsValid())
			{
				Pair.Key->SetDirtyFlag(false);
			}
		}
	};

	TSet<FName> SelectedSet;
	for (FName SlotId : SelectedSlotIds)
	{
		SelectedSet.Add(SlotId);
	}

	int32 LoadedPreviewCount = 0;
	int32 ConfiguredPCGCount = 0;
	for (AGP_VillageSlot* Slot : CachedSlots)
	{
		if (!IsValid(Slot) || !SelectedSet.Contains(Slot->GetSlotId()))
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
				TEXT("[VillageLayout] No valid editor preview preset is assigned to slot '%s'."),
				*SlotId.ToString());
			ClearEditorVillagePreview();
			return false;
		}

		const FTransform PreviewTransform(
			Slot->GetActorRotation(),
			Slot->GetActorLocation(),
			FVector::OneVector);
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.OverrideLevel = World->PersistentLevel;
		SpawnParams.ObjectFlags = RF_Transient | RF_DuplicateTransient;
		SpawnParams.bCreateActorPackage = false;
		SpawnParams.bHideFromSceneOutliner = true;
		SpawnParams.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.CustomPreSpawnInitalization =
			[AssignedLevel](AActor* SpawnedActor)
			{
				ALevelInstance* LevelInstance = CastChecked<ALevelInstance>(SpawnedActor);
				LevelInstance->bIsEditorOnlyActor = true;
				LevelInstance->SetLockLocation(true);
				LevelInstance->SetDesiredRuntimeBehavior(
					ELevelInstanceRuntimeBehavior::LevelStreaming);
				LevelInstance->SetWorldAsset(AssignedLevel);
			};

		ALevelInstance* PreviewInstance = World->SpawnActor<ALevelInstance>(
			ALevelInstance::StaticClass(),
			PreviewTransform,
			SpawnParams);
		if (!PreviewInstance)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[VillageLayout] Failed to spawn editor preview for slot '%s'."),
				*SlotId.ToString());
			ClearEditorVillagePreview();
			return false;
		}

		PreviewInstance->SetActorLabel(
			FString::Printf(
				TEXT("PREVIEW_%s_%s"),
				*SlotId.ToString(),
				*AssignedPresetId.ToString()),
			false);
		EditorPreviewVillageInstances.Add(PreviewInstance);
		LevelInstanceSubsystem->BlockLoadLevelInstance(PreviewInstance);

		ULevelStreamingLevelInstance* StreamingLevel = PreviewInstance->GetLevelStreaming();
		if (!StreamingLevel || !StreamingLevel->GetLoadedLevel())
		{
			UE_LOG(LogTemp, Error,
				TEXT("[VillageLayout] Editor preview level failed to load for slot '%s'."),
				*SlotId.ToString());
			ClearEditorVillagePreview();
			return false;
		}

		StreamingLevel->SetFlags(RF_DuplicateTransient);
		UPackage* PreviewLevelPackage = StreamingLevel->GetLoadedLevel()->GetOutermost();
		PreviewPackageDirtyStates.FindOrAdd(
			PreviewLevelPackage,
			PreviewLevelPackage ? PreviewLevelPackage->IsDirty() : false);
		++LoadedPreviewCount;

		if (!bGeneratePCGInEditorPreview)
		{
			continue;
		}

		UPCGComponent* PCGComponent = FindVillagePCGComponent(StreamingLevel);
		if (!PCGComponent)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[VillageLayout] Could not find the city PCG component for preview slot '%s'."),
				*SlotId.ToString());
			ClearEditorVillagePreview();
			return false;
		}

		// Mark the component as preview-owned before changing tags, graph parameters,
		// or generation settings so none of those edits become authored data.
		PCGComponent->SetEditingMode(
			EPCGEditorDirtyMode::Preview,
			PCGComponent->GetSerializedEditingMode());

		int32 PCGComponentCount = 0;
		int32 RoadActorCount = 0;
		int32 DistrictActorCount = 0;
		if (!ConfigureVillagePCG(
			StreamingLevel,
			SlotId,
			PCGComponentCount,
			RoadActorCount,
			DistrictActorCount))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[VillageLayout] Failed to configure editor preview PCG for slot '%s'."),
				*SlotId.ToString());
			ClearEditorVillagePreview();
			return false;
		}

		EditorPreviewPCGComponents.Add(PCGComponent);
		ConfiguredPCGCount += PCGComponentCount;
	}

	if (LoadedPreviewCount != SelectedSlotIds.Num())
	{
		UE_LOG(LogTemp, Error,
			TEXT("[VillageLayout] Loaded %d of %d selected editor preview levels."),
			LoadedPreviewCount,
			SelectedSlotIds.Num());
		ClearEditorVillagePreview();
		return false;
	}

	if (bGeneratePCGInEditorPreview)
	{
		EditorPreviewSynchronousGeneratedComponents.Reset();
		EditorPreviewSynchronousCancelledComponents.Reset();
		FPCGTaskId PreviousTaskId = InvalidPCGTaskId;
		for (UPCGComponent* PCGComponent : EditorPreviewPCGComponents)
		{
			if (!IsValid(PCGComponent))
			{
				ClearEditorVillagePreview();
				return false;
			}

			TArray<FPCGTaskId> Dependencies;
			if (PreviousTaskId != InvalidPCGTaskId)
			{
				Dependencies.Add(PreviousTaskId);
			}

			PCGComponent->OnPCGGraphGeneratedDelegate.RemoveAll(this);
			PCGComponent->OnPCGGraphCancelledDelegate.RemoveAll(this);
			PCGComponent->OnPCGGraphGeneratedDelegate.AddUObject(
				this,
				&AGP_VillageLayoutDirector::HandleEditorPreviewPCGGenerated);
			PCGComponent->OnPCGGraphCancelledDelegate.AddUObject(
				this,
				&AGP_VillageLayoutDirector::HandleEditorPreviewPCGCancelled);
			const FPCGTaskId TaskId = PCGComponent->GenerateLocalGetTaskId(
				EPCGComponentGenerationTrigger::GenerateOnDemand,
				true,
				EPCGHiGenGrid::Uninitialized,
				Dependencies);
			const bool bCompletedSynchronously =
				EditorPreviewSynchronousGeneratedComponents.Contains(PCGComponent);
			const bool bCancelledSynchronously =
				EditorPreviewSynchronousCancelledComponents.Contains(PCGComponent);
			PCGComponent->OnPCGGraphGeneratedDelegate.RemoveAll(this);
			PCGComponent->OnPCGGraphCancelledDelegate.RemoveAll(this);
			if (bCancelledSynchronously
				|| (TaskId == InvalidPCGTaskId && !bCompletedSynchronously))
			{
				UE_LOG(LogTemp, Error,
					TEXT("[VillageLayout] Failed to schedule an editor preview PCG graph."));
				ClearEditorVillagePreview();
				return false;
			}
			PreviousTaskId = TaskId;
		}
		EditorPreviewSynchronousGeneratedComponents.Reset();
		EditorPreviewSynchronousCancelledComponents.Reset();
	}

	UE_LOG(LogTemp, Log,
		TEXT("[VillageLayout] Editor preview loaded %d transient village level(s); PCG=%s Components=%d."),
		LoadedPreviewCount,
		bGeneratePCGInEditorPreview ? TEXT("ScheduledSequentially") : TEXT("Disabled"),
		ConfiguredPCGCount);
	return true;
#else
	return false;
#endif
}

void AGP_VillageLayoutDirector::ClearEditorVillagePreview()
{
#if WITH_EDITOR
	UWorld* World = GetWorld();
	if (!World || World->IsGameWorld())
	{
		return;
	}

	const bool bWasWorldPackageDirty = World->GetOutermost()->IsDirty();
	ON_SCOPE_EXIT
	{
		if (!bWasWorldPackageDirty && IsValid(World))
		{
			World->GetOutermost()->SetDirtyFlag(false);
		}
	};

	TSet<UPCGComponent*> CleanedPCGComponents;
	ULevelInstanceSubsystem* LevelInstanceSubsystem = World->GetSubsystem<ULevelInstanceSubsystem>();
	for (ALevelInstance* PreviewInstance : EditorPreviewVillageInstances)
	{
		if (!IsValid(PreviewInstance)
			|| PreviewInstance->GetOwner() != this
			|| PreviewInstance->GetWorld() != World)
		{
			continue;
		}

		if (ULevel* LoadedLevel = PreviewInstance->GetLoadedLevel())
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
					if (IsValid(PCGComponent) && !CleanedPCGComponents.Contains(PCGComponent))
					{
						CleanedPCGComponents.Add(PCGComponent);
						PCGComponent->OnPCGGraphGeneratedDelegate.RemoveAll(this);
						PCGComponent->OnPCGGraphCancelledDelegate.RemoveAll(this);
						PCGComponent->CancelGeneration();
						PCGComponent->CleanupLocalImmediate(true);
					}
				}
			}
		}

		if (LevelInstanceSubsystem)
		{
			LevelInstanceSubsystem->BlockUnloadLevelInstance(PreviewInstance);
		}
		if (IsValid(PreviewInstance))
		{
			World->DestroyActor(PreviewInstance, false, false);
		}
	}

	if (!EditorPreviewVillageInstances.IsEmpty())
	{
		UE_LOG(LogTemp, Log,
			TEXT("[VillageLayout] Cleared %d transient editor village preview level(s)."),
			EditorPreviewVillageInstances.Num());
	}
	EditorPreviewPCGComponents.Reset();
	EditorPreviewVillageInstances.Reset();
	EditorPreviewSynchronousGeneratedComponents.Reset();
	EditorPreviewSynchronousCancelledComponents.Reset();
#endif
}

#if WITH_EDITOR
void AGP_VillageLayoutDirector::HandleEditorPreviewPCGGenerated(UPCGComponent* PCGComponent)
{
	if (IsValid(PCGComponent))
	{
		EditorPreviewSynchronousGeneratedComponents.Add(PCGComponent);
	}
}

void AGP_VillageLayoutDirector::HandleEditorPreviewPCGCancelled(UPCGComponent* PCGComponent)
{
	if (IsValid(PCGComponent))
	{
		EditorPreviewSynchronousCancelledComponents.Add(PCGComponent);
	}
}
#endif
