#pragma once

#include "Commandlets/Commandlet.h"
#include "GP_ApplySansBossAnimationSetupCommandlet.generated.h"

// Editor commandlet that rebuilds the Sans boss animation assets without opening the full editor UI.
UCLASS()
class PROJECT_EDEN_API UGP_ApplySansBossAnimationSetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGP_ApplySansBossAnimationSetupCommandlet();

	virtual int32 Main(const FString& Params) override;
};
