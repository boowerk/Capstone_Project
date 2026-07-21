#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Game/WorldLayout/GP_VillageTypes.h"
#include "GP_VillageSlot.generated.h"

class AGP_VillageSlot;
class UBoxComponent;

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
	void SetSelectedForRun(bool bSelected);

	FName GetSlotId() const { return SlotId; }
	FName GetGroupId() const { return GroupId; }
	bool IsCandidateEnabled() const { return bEnabled; }
	bool IsSelectedForRun() const { return bSelectedForRun; }
	UBoxComponent* GetSlotBounds() const { return SlotBounds; }

	UPROPERTY(BlueprintAssignable, Category = "Village")
	FGPOnVillageSlotSelectionChanged OnSelectionChanged;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Village")
	TObjectPtr<UBoxComponent> SlotBounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Village")
	FName SlotId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Village")
	FName GroupId = TEXT("Village");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Village", meta = (ClampMin = "0.0"))
	float SelectionWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Village")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Village", meta = (ClampMin = "-1"))
	int32 RegionId = INDEX_NONE;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Village")
	bool bSelectedForRun = false;

#if WITH_EDITOR
	virtual bool ActorTypeSupportsDataLayer() const override { return false; }
	virtual bool ActorTypeSupportsExternalDataLayer() const override { return false; }
#endif
};
