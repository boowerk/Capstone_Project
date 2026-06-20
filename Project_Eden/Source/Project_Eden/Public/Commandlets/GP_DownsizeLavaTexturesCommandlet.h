#pragma once

#include "Commandlets/Commandlet.h"

#include "GP_DownsizeLavaTexturesCommandlet.generated.h"

UCLASS()
class PROJECT_EDEN_API UGP_DownsizeLavaTexturesCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGP_DownsizeLavaTexturesCommandlet();

	virtual int32 Main(const FString& Params) override;
};
