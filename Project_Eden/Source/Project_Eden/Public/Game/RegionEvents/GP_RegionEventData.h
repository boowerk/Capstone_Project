#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GP_RegionEventData.generated.h"

/**
 * Serialization-only compatibility type for the protected legacy DA_RegionEventData asset.
 *
 * Scripted encounter fields were removed with the fixed demo flow, so this asset cannot configure
 * or start gameplay and may be deleted later when the protected content is intentionally migrated.
 */
UCLASS(BlueprintType)
class PROJECT_EDEN_API UGP_RegionEventData : public UDataAsset
{
	GENERATED_BODY()
};
