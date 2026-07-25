#include "Game/WorldLayout/GP_VillageLayoutDirector.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Level.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Misc/Crc.h"
#include "PCGComponent.h"
#include "PCGGraph.h"
#include "PCGManagedResource.h"

namespace
{
	constexpr TCHAR CityPCGGraphObjectPath[] =
		TEXT("/Game/RegionSystem/PCG/CityGen/PCG_CityGen_FromAnchor.PCG_CityGen_FromAnchor");
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

FName AGP_VillageLayoutDirector::MakeVillageRoleTag(FName SlotId, const TCHAR* RoleSuffix) const
{
	const FName* ActiveInstanceName = ActiveVillageInstanceNames.Find(SlotId);
	const FString Prefix = ActiveInstanceName
		? ActiveInstanceName->ToString()
		: MakeStreamingLevelName(SlotId);
	return FName(*FString::Printf(TEXT("%s_%s"), *Prefix, RoleSuffix));
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
