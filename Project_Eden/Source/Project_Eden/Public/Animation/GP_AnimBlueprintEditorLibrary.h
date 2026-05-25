#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GP_AnimBlueprintEditorLibrary.generated.h"

class UAnimBlueprint;
class UChooserTable;

UCLASS()
class PROJECT_EDEN_API UGP_AnimBlueprintEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "ProjectEden|Animation|Editor")
	static bool ConfigureChooserPlayer(UAnimBlueprint* AnimBlueprint, UChooserTable* ChooserTable, bool bCompileBlueprint = true);

	UFUNCTION(BlueprintCallable, Category = "ProjectEden|Animation|Editor")
	static bool RestoreMotionMatchingOutput(UAnimBlueprint* AnimBlueprint, bool bCompileBlueprint = true);

	UFUNCTION(BlueprintCallable, Category = "ProjectEden|Animation|Editor")
	static bool BindMotionMatchingUpdateFunction(UAnimBlueprint* AnimBlueprint, FName FunctionName, bool bCompileBlueprint = true);

	UFUNCTION(BlueprintCallable, Category = "ProjectEden|Animation|Editor")
	static bool BindMotionMatchingFullUpdate(UAnimBlueprint* AnimBlueprint, FName FunctionName, bool bCompileBlueprint = true);
};
