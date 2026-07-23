#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP_RegionEventDirector.generated.h"

/**
 * Serialization-only compatibility actor for BP_EventDirector already placed in L_LandscapeMap.
 *
 * The fixed demo and region-event runtime were removed. This type intentionally exposes no
 * scheduling, spawning, objective, reward, or completion API and performs no gameplay work.
 */
UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_RegionEventDirector : public AActor
{
	GENERATED_BODY()

public:
	AGP_RegionEventDirector();
};
