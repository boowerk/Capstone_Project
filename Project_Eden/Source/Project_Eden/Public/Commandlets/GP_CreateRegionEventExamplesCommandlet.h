#pragma once

#include "Commandlets/Commandlet.h"
#include "GP_CreateRegionEventExamplesCommandlet.generated.h"

// Editor commandlet that creates the four region event example BP/DataAsset assets for PIE smoke testing.
UCLASS()
class PROJECT_EDEN_API UGP_CreateRegionEventExamplesCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGP_CreateRegionEventExamplesCommandlet();

	virtual int32 Main(const FString& Params) override;
};
