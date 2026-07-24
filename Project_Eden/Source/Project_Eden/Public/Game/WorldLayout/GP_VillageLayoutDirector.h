#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Game/WorldLayout/GP_VillageTypes.h"
#include "TimerManager.h"
#include "GP_VillageLayoutDirector.generated.h"

class AGP_VillageSlot;
class ALevelInstance;
class UPCGComponent;
class UGP_VillagePresetCatalog;
class USceneComponent;
class ULevelStreamingDynamic;
class UWorld;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGPOnVillageRuntimeLayoutReady);

UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_VillageLayoutDirector : public AActor
{
	GENERATED_BODY()

public:
	AGP_VillageLayoutDirector();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Village")
	bool BuildSelectionForSeed(int32 InRunSeed);

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Village|Debug")
	void RebuildPreview();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Village|Debug")
	void ClearVillagePreview();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Village|Footprint")
	void RefreshFootprintPreview();
	void RefreshFootprintPreviewIgnoringSlot(const AGP_VillageSlot* IgnoredSlot);

	UFUNCTION(BlueprintPure, Category = "Village")
	TArray<FName> GetSelectedSlotIds() const { return SelectedSlotIds; }

	UFUNCTION(BlueprintPure, Category = "Village|Debug")
	FString GetLastSelectionSummary() const { return LastSelectionSummary; }

	UFUNCTION(BlueprintPure, Category = "Village|Runtime")
	bool IsRuntimeLayoutReady() const { return bRuntimeLayoutReady; }

	UFUNCTION(BlueprintPure, Category = "Village|Runtime")
	int32 GetRuntimeLayoutRevision() const { return RuntimeLayoutSnapshot.Revision; }

	UPROPERTY(BlueprintAssignable, Category = "Village|Runtime")
	FGPOnVillageRuntimeLayoutReady OnRuntimeLayoutReady;

	UFUNCTION(BlueprintPure, Category = "Village|Level Instance")
	TSoftObjectPtr<UWorld> GetVillageLevelPreset() const { return VillageLevelPreset; }

	UFUNCTION(BlueprintPure, Category = "Village|Footprint")
	FGP_VillageFootprint GetVillagePresetFootprint() const { return VillagePresetFootprint; }

	UFUNCTION(BlueprintPure, Category = "Village|Level Instance")
	TArray<FGP_VillagePresetDefinition> GetVillagePresetPool() const;

	UFUNCTION(BlueprintPure, Category = "Village|Level Instance")
	UGP_VillagePresetCatalog* GetVillagePresetCatalog() const
	{
		return VillagePresetCatalog;
	}

	static int32 SelectVillagePresetIndex(
		int32 InRunSeed,
		FName SlotId,
		const TArray<FGP_VillagePresetDefinition>& Presets,
		int32 AssignmentAttempt = 0,
		EGP_VillageSlotSizeClass SlotSizeClass = EGP_VillageSlotSizeClass::Medium);
	static TArray<FGP_VillagePresetDefinition> MergeVillagePresetSources(
		const TArray<FGP_VillagePresetDefinition>& PrimaryPresets,
		const TArray<FGP_VillagePresetDefinition>& CatalogPresets,
		const TArray<FGP_VillagePresetDefinition>& LegacyPresets);
	static bool DoesVillageFootprintFitSlot(
		const FGP_VillageFootprint& Footprint,
		EGP_VillageSlotSizeClass SlotSizeClass);
	static bool DoVillageFootprintsOverlap(
		const FTransform& FirstSlotTransform,
		const FGP_VillageFootprint& FirstFootprint,
		const FTransform& SecondSlotTransform,
		const FGP_VillageFootprint& SecondFootprint);

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Destroyed() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Village")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Village")
	bool bAutoBuildOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Village")
	TArray<FGP_VillageGroupRule> GroupRules;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Village|Data Layer")
	bool bApplyDataLayersAtRuntime = false;

	// Selected slots instance the same authored village map independently.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Village|Level Instance")
	TSoftObjectPtr<UWorld> VillageLevelPreset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Village|Level Instance", meta = (ShowOnlyInnerProperties))
	FGP_VillageFootprint VillagePresetFootprint;

	// Content-owned preset list. Add new village levels to this asset rather than
	// changing the Director defaults or resaving every map that contains one.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Village|Level Instance")
	TObjectPtr<UGP_VillagePresetCatalog> VillagePresetCatalog = nullptr;

	// Legacy per-Director additions. The shared catalog is authoritative when an
	// ID or level path is duplicated; unique legacy entries are still appended.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Village|Level Instance", meta = (TitleProperty = "PresetId"))
	TArray<FGP_VillagePresetDefinition> VillagePresetPool;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Village|Level Instance")
	bool bStreamVillageLevelAtRuntime = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Village|Level Instance", meta = (ClampMin = "1.0", Units = "s"))
	float VillageStreamingTimeoutSeconds = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Village|Debug", meta = (ClampMin = "0"))
	int32 PreviewSeed = 1337;

	// Keeps PIE village placement identical to RebuildPreview without changing
	// the packaged game's RunSeed behavior.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Village|Debug")
	bool bUsePreviewSeedInPIE = true;

	// RebuildPreview loads transient Level Instances in the editor and optionally
	// runs their city PCG graphs so the authored village layout is visible before PIE.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Village|Debug")
	bool bGeneratePCGInEditorPreview = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Village|Debug")
	bool bDrawDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Village|Debug", meta = (ClampMin = "0.0", Units = "s"))
	float DebugDrawDuration = 30.0f;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<AGP_VillageSlot>> CachedSlots;

	UPROPERTY(Transient)
	TArray<FName> SelectedSlotIds;

	UPROPERTY(Transient)
	FString LastSelectionSummary;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<ULevelStreamingDynamic>> ActiveVillageLevels;

	UPROPERTY(Transient)
	TMap<FName, int32> AssignedVillagePresetIndices;

	// The server publishes the exact selected level, transform, and stable instance
	// name. Each client then generates the non-replicated visual PCG locally.
	UPROPERTY(ReplicatedUsing = OnRep_RuntimeLayoutSnapshot)
	FGP_VillageRuntimeLayoutSnapshot RuntimeLayoutSnapshot;

#if WITH_EDITORONLY_DATA
	UPROPERTY(Transient, DuplicateTransient)
	TArray<TObjectPtr<ALevelInstance>> EditorPreviewVillageInstances;

	UPROPERTY(Transient, DuplicateTransient)
	TArray<TObjectPtr<UPCGComponent>> EditorPreviewPCGComponents;
#endif

	TSet<FName> ConfiguredVillageLevelIds;
	TSet<FName> GenerationRequestedVillageLevelIds;
	TSet<FName> GeneratedVillageLevelIds;
	TMap<UPCGComponent*, FName> PendingVillagePCGComponents;
	TMap<FName, int32> ActiveVillagePCGSeeds;
	TMap<FName, FName> ActiveVillageInstanceNames;
	FTimerHandle VillageStreamingTimeoutHandle;
	int32 ExpectedVillageLevelCount = 0;
	bool bVillageLevelBatchFailed = false;
	bool bSchedulingVillagePCG = false;
	bool bRuntimeLayoutReady = false;

	int32 LastRunSeed = INDEX_NONE;
	int32 LastPresetAssignmentAttempt = 0;
	int32 AppliedRuntimeLayoutRevision = 0;

	void CollectSlots();
	void AssignVillagePresets(int32 InRunSeed, int32 AssignmentAttempt = 0);
	TArray<FGP_VillagePresetDefinition> BuildEffectiveVillagePresetPool() const;
	void ApplyFootprintPreview();
	bool BuildEditorVillagePreview();
	void ClearEditorVillagePreview();
	UPCGComponent* FindVillagePCGComponent(ULevelStreamingDynamic* StreamingLevel) const;
	bool ResolveVillagePreset(
		FName SlotId,
		TSoftObjectPtr<UWorld>& OutLevel,
		FGP_VillageFootprint& OutFootprint,
		FName& OutPresetId) const;
	void PopulateCandidateOverlaps(
		TArray<FGP_VillageCandidate>& InOutCandidates,
		const TArray<AGP_VillageSlot*>& CandidateSlots) const;
	bool ApplyDataLayerStates(const TSet<FName>& InSelectedSlotIds) const;
	bool StreamSelectedVillageLevels(const TSet<FName>& InSelectedSlotIds);
	bool BuildRuntimeLayoutInstances(
		const TSet<FName>& InSelectedSlotIds,
		TArray<FGP_VillageRuntimeInstance>& OutInstances) const;
	bool StreamRuntimeVillageLevels(const TArray<FGP_VillageRuntimeInstance>& Instances);
	void PublishRuntimeLayoutSnapshot(
		const TArray<FGP_VillageRuntimeInstance>& Instances,
		bool bShouldLoad);
	void PublishEmptyRuntimeLayoutSnapshot();
	void TryApplyReplicatedRuntimeLayout();
	void UnloadActiveVillageLevels();
	bool ConfigureVillagePCG(
		ULevelStreamingDynamic* StreamingLevel,
		FName SlotId,
		int32& OutPCGComponentCount,
		int32& OutRoadActorCount,
		int32& OutDistrictActorCount) const;
	void ConfigureVillageGameplayActors(
		ULevelStreamingDynamic* StreamingLevel,
		FName SlotId) const;
	bool GenerateVillagePCG(
		ULevelStreamingDynamic* StreamingLevel,
		FName SlotId,
		int32& OutPCGComponentCount);
	bool RequestNextVillagePCGGeneration();
	void LogVillagePCGResult(FName SlotId, UPCGComponent* PCGComponent) const;
	bool GetCityPCGTemplateTags(
		const UPCGComponent* PCGComponent,
		FName& OutRoadTag,
		FName& OutDistrictTag) const;
	int32 MakeVillagePCGSeed(FName SlotId) const;
	int32 GetNextRuntimeLayoutRevision() const;
	FString MakeStreamingLevelName(FName SlotId) const;
	FName MakeVillageRoleTag(FName SlotId, const TCHAR* RoleSuffix) const;
	void HandleVillagePCGGenerated(UPCGComponent* PCGComponent);
	void HandleVillagePCGCancelled(UPCGComponent* PCGComponent);
	void HandleVillageStreamingTimeout();
	void ClearVillageStreamingTimeout();
	void MarkRuntimeLayoutReady();
	void DrawSelectionDebug() const;

	UFUNCTION()
	void HandleVillageLevelLoaded();

	UFUNCTION()
	void HandleVillageLevelShown();

	UFUNCTION()
	void OnRep_RuntimeLayoutSnapshot();

#if WITH_EDITOR
	void HandleEditorPreviewPCGGenerated(UPCGComponent* PCGComponent);
	void HandleEditorPreviewPCGCancelled(UPCGComponent* PCGComponent);
	virtual bool ActorTypeSupportsDataLayer() const override { return false; }
	virtual bool ActorTypeSupportsExternalDataLayer() const override { return false; }

	TSet<TObjectPtr<UPCGComponent>> EditorPreviewSynchronousGeneratedComponents;
	TSet<TObjectPtr<UPCGComponent>> EditorPreviewSynchronousCancelledComponents;
#endif
};
