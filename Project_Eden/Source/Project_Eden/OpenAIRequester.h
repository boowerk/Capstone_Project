#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "OpenAIRequester.generated.h"

UCLASS(BlueprintType, Blueprintable)
class PROJECT_EDEN_API UOpenAIRequester : public UObject
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category = "OpenAI")
    void SendOpenAIRequest();
};
