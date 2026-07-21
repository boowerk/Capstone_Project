#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Game/WorldLayout/GP_VillageTypes.h"
#include "GP_VillageLayoutDirector.generated.h"

class AGP_VillageSlot;
class USceneComponent;
class ULevelStreamingDynamic;
class UWorld;

UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_VillageLayoutDirector : public AActor
{
	GENERATED_BODY()

public:
	AGP_VillageLayoutDirector();

	UFUNCTION(BlueprintCallable, Category = "Village")
	bool BuildSelectionForSeed(int32 InRunSeed);

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Village|Debug")
	void RebuildPreview();

	UFUNCTION(BlueprintPure, Category = "Village")
	TArray<FName> GetSelectedSlotIds() const { return SelectedSlotIds; }

	UFUNCTION(BlueprintPure, Category = "Village|Debug")
	FString GetLastSelectionSummary() const { return LastSelectionSummary; }

	UFUNCTION(BlueprintPure, Category = "Village|Level Instance")
	TSoftObjectPtr<UWorld> GetVillageLevelPreset() const { return VillageLevelPreset; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Village")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Village")
	bool bAutoBuildOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Village")
	TArray<FGP_VillageGroupRule> GroupRules;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Village|Data Layer")
	bool bApplyDataLayersAtRuntime = false;

	// V1 uses one authored village map at one selected slot per run.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Village|Level Instance")
	TSoftObjectPtr<UWorld> VillageLevelPreset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Village|Level Instance")
	bool bStreamVillageLevelAtRuntime = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Village|Debug", meta = (ClampMin = "0"))
	int32 PreviewSeed = 1337;

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

	TSet<FName> ConfiguredVillageLevelIds;
	TSet<FName> GeneratedVillageLevelIds;

	int32 LastRunSeed = INDEX_NONE;

	void CollectSlots();
	bool ApplyDataLayerStates(const TSet<FName>& InSelectedSlotIds) const;
	bool StreamSelectedVillageLevel(const TSet<FName>& InSelectedSlotIds);
	void UnloadActiveVillageLevels();
	int32 ConfigureVillagePCG(ULevelStreamingDynamic* StreamingLevel, FName SlotId) const;
	int32 GenerateVillagePCG(ULevelStreamingDynamic* StreamingLevel) const;
	int32 MakeVillagePCGSeed(FName SlotId) const;
	FString MakeStreamingLevelName(FName SlotId) const;
	void DrawSelectionDebug() const;

	UFUNCTION()
	void HandleVillageLevelLoaded();

	UFUNCTION()
	void HandleVillageLevelShown();

#if WITH_EDITOR
	virtual bool ActorTypeSupportsDataLayer() const override { return false; }
	virtual bool ActorTypeSupportsExternalDataLayer() const override { return false; }
#endif
};
