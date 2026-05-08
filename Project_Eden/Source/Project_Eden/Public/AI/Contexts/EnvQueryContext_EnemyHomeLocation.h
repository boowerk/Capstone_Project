#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "EnvQueryContext_EnemyHomeLocation.generated.h"

struct FEnvQueryContextData;
struct FEnvQueryInstance;

UCLASS()
class PROJECT_EDEN_API UEnvQueryContext_EnemyHomeLocation : public UEnvQueryContext
{
	GENERATED_BODY()

public:
	// Exposes the enemy's behavior anchor as an EQS point context for patrol queries.
	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;
};
