#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Game/WorldLayout/GP_VillageTypes.h"
#include "GP_VillagePresetCatalog.generated.h"

/**
 * Content-owned list of village level presets.
 *
 * Add new village levels here so the layout director can discover them without
 * requiring a C++ default change or a resave of the landscape map.
 */
UCLASS(BlueprintType)
class PROJECT_EDEN_API UGP_VillagePresetCatalog : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Village", meta = (TitleProperty = "PresetId"))
	TArray<FGP_VillagePresetDefinition> Presets;
};
