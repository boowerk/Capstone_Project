#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "GP_DamageExecCalculation.generated.h"

UCLASS()
class PROJECT_EDEN_API UGP_DamageExecCalculation : public UGameplayEffectExecutionCalculation
{
    GENERATED_BODY()

public:
    UGP_DamageExecCalculation();

    virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, OUT FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};