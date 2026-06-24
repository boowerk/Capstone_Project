#pragma once

#include "Commandlets/Commandlet.h"
#include "GP_ApplyCrystalSeraphAnimationSetupCommandlet.generated.h"

// Editor commandlet that rebuilds the Crystal Seraph animation assets and assigns them to the boss Blueprint.
UCLASS()
class PROJECT_EDEN_API UGP_ApplyCrystalSeraphAnimationSetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGP_ApplyCrystalSeraphAnimationSetupCommandlet();

	virtual int32 Main(const FString& Params) override;
};
