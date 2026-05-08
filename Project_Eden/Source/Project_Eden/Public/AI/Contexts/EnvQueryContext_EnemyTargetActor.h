#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "EnvQueryContext_EnemyTargetActor.generated.h"

struct FEnvQueryContextData;
struct FEnvQueryInstance;

UCLASS()
class PROJECT_EDEN_API UEnvQueryContext_EnemyTargetActor : public UEnvQueryContext
{
	GENERATED_BODY()

public:
	// 공유 Blackboard의 TargetActor를 EQS Context로 노출한다.
	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;
};
