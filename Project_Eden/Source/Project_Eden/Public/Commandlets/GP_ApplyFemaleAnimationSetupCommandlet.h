#pragma once

#include "Commandlets/Commandlet.h"

#include "GP_ApplyFemaleAnimationSetupCommandlet.generated.h"

UCLASS()
class PROJECT_EDEN_API UGP_ApplyFemaleAnimationSetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGP_ApplyFemaleAnimationSetupCommandlet();

	virtual int32 Main(const FString& Params) override;
};
