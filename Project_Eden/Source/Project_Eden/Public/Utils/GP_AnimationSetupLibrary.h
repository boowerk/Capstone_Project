#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "GP_AnimationSetupLibrary.generated.h"

UCLASS()
class PROJECT_EDEN_API UGP_AnimationSetupLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "ProjectEden|Editor")
	static bool CreateFemalePlayerAnimationSetup();

	UFUNCTION(BlueprintCallable, Category = "ProjectEden|Editor")
	static bool CreateDashInputSetup();
};
