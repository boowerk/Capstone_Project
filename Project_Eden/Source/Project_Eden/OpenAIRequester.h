#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PcgDataTypes.h"
#include "OpenAIRequester.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScatterParams, FPCGScatterParams, Params);

UCLASS(BlueprintType, Blueprintable)
class PROJECT_EDEN_API UOpenAIRequester : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable, Category = "OpenAI")
        FOnScatterParams OnScatterParams;

    UFUNCTION(BlueprintCallable, Category = "OpenAI")
        void SendOpenAIRequest();

private:
    FString BuildPayloadJSON() const;
};
