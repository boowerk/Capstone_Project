#pragma once

#include "CoreMinimal.h"

namespace GPCrystalSeraphVFXDefaults
{
	FORCEINLINE FLinearColor GetCrystalTintColor()
	{
		// 59ADFFFF is the Crystal Seraph-specific tint requested for duplicated Niagara systems.
		return FLinearColor(0.34901962f, 0.67843139f, 1.0f, 1.0f);
	}
}
