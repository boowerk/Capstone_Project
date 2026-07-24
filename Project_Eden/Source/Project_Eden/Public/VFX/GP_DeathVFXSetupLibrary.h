#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GP_DeathVFXSetupLibrary.generated.h"

class UNiagaraSystem;

/** Narrow editor setup helpers for production death VFX assets. */
UCLASS()
class PROJECT_EDEN_API UGP_DeathVFXSetupLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Exposes a Material user parameter and binds every Sprite Renderer in the
	 * supplied Niagara System to it. Runtime components can then provide a
	 * different particle material for each enemy without duplicating the system.
	 */
	UFUNCTION(BlueprintCallable, Category = "Editor|Death VFX", meta = (DevelopmentOnly))
	static bool ConfigureAbsorptionMaterialBinding(
		UNiagaraSystem* NiagaraSystem,
		FName MaterialParameterName);
};
