#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Game/WorldLayout/GP_VillageTypes.h"
#include "GP_VillageSlot.generated.h"

class AGP_VillageSlot;
class UBoxComponent;
class UDataLayerAsset;
struct FPropertyChangedEvent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FGPOnVillageSlotSelectionChanged,
	AGP_VillageSlot*, Slot,
	bool, bSelected);

UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_VillageSlot : public AActor
{
	GENERATED_BODY()

public:
	AGP_VillageSlot();

	FGP_VillageCandidate MakeCandidate() const;
	void ApplyFootprint(const FGP_VillageFootprint& Footprint);
	void ClearFootprintPreview();
	void SetFootprintConflict(bool bConflicting);
	void SetSelectedForRun(bool bSelected);

	FName GetSlotId() const { return SlotId; }
	FName GetGroupId() const { return GroupId; }
	EGP_VillageSlotSizeClass GetSlotSizeClass() const { return SlotSizeClass; }
	bool IsCandidateEnabled() const { return bEnabled; }
	bool IsSelectedForRun() const { return bSelectedForRun; }
	bool HasFootprintConflict() const { return bFootprintConflict; }
	UBoxComponent* GetSlotBounds() const { return FootprintBounds; }
	UBoxComponent* GetCapacityBounds() const { return CapacityBounds; }
	UDataLayerAsset* GetVillageDataLayer() const { return VillageDataLayer; }
	FColor GetPreviewColor() const;

	UPROPERTY(BlueprintAssignable, Category = "Village")
	FGPOnVillageSlotSelectionChanged OnSelectionChanged;

protected:
	virtual void Destroyed() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Village")
	TObjectPtr<UBoxComponent> SlotBounds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Village|Capacity")
	TObjectPtr<UBoxComponent> CapacityBounds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Village|Footprint")
	TObjectPtr<UBoxComponent> FootprintBounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Village")
	FName SlotId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Village")
	FName GroupId = TEXT("Village");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Village", meta = (ClampMin = "0.0"))
	float SelectionWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Village|Capacity")
	EGP_VillageSlotSizeClass SlotSizeClass = EGP_VillageSlotSizeClass::Medium;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Village")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Village", meta = (ClampMin = "-1"))
	int32 RegionId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Village|Data Layer")
	TObjectPtr<UDataLayerAsset> VillageDataLayer;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Village")
	bool bSelectedForRun = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Village|Footprint")
	bool bFootprintConflict = false;

#if WITH_EDITOR
	virtual void PostEditMove(bool bFinished) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditUndo() override;
	virtual bool ActorTypeSupportsDataLayer() const override { return false; }
	virtual bool ActorTypeSupportsExternalDataLayer() const override { return false; }
#endif

private:
	void UpdateCapacityPreview();
	void UpdatePreviewColor();

#if WITH_EDITOR
	void NotifyLayoutDirectorsFootprintChanged() const;
#endif
};
