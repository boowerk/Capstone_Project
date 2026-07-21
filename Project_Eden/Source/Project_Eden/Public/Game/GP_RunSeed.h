#pragma once

#include "CoreMinimal.h"

namespace GPRunSeed
{
	PROJECT_EDEN_API int32 Generate();
	PROJECT_EDEN_API bool TryParse(const FString& Options, int32& OutRunSeed);
	PROJECT_EDEN_API FString BuildTravelURL(const FString& GameMapName, int32 RunSeed);
}
