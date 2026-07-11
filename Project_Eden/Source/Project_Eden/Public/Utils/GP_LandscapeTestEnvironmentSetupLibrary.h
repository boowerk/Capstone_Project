#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GP_LandscapeTestEnvironmentSetupLibrary.generated.h"

/** Editor utility that idempotently prepares L_LandscapeMap for corruption and Region Event smoke tests. */
UCLASS()
class PROJECT_EDEN_API UGP_LandscapeTestEnvironmentSetupLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Project Eden|Testing|Landscape", meta = (DevelopmentOnly))
	static bool CreateOrUpdateLandscapeTestEnvironment();
};
