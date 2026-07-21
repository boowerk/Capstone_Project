#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Game/WorldLayout/GP_VillageTypes.h"
#include "GP_VillageLayoutDirector.generated.h"

class AGP_VillageSlot;
class USceneComponent;

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

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Village")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Village")
	bool bAutoBuildOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Village")
	TArray<FGP_VillageGroupRule> GroupRules;

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

	int32 LastRunSeed = INDEX_NONE;

	void CollectSlots();
	void DrawSelectionDebug() const;

#if WITH_EDITOR
	virtual bool ActorTypeSupportsDataLayer() const override { return false; }
	virtual bool ActorTypeSupportsExternalDataLayer() const override { return false; }
#endif
};
