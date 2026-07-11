#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GP_CreateLandscapeTestEnvironmentCommandlet.generated.h"

/** Headless entry point for regenerating the committed L_LandscapeMap smoke-test environment. */
UCLASS()
class PROJECT_EDEN_API UGP_CreateLandscapeTestEnvironmentCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGP_CreateLandscapeTestEnvironmentCommandlet();

	virtual int32 Main(const FString& Params) override;
};
