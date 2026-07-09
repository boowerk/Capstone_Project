#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GP_RegionEventExampleSetupLibrary.generated.h"

/** Editor-only helper that creates the Blueprint/DataAsset examples used to smoke-test Region Events. */
UCLASS()
class PROJECT_EDEN_API UGP_RegionEventExampleSetupLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Rebuilds the four region-event example Blueprints, their DataAssets, and a test director Blueprint. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Project Eden|Region Events")
	static bool CreateOrUpdateRegionEventExampleAssets();
};
