#pragma once
#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include "UObject/NoExportTypes.h"
#include "PCG/PcgDataTypes.h"
#include "PlayerBehaviorTreeBuilder.h"
#include "OpenAIRequester.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScatterParams, FPCGScatterParams, Params);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerEvaluationReady, const FPlayerEvaluationSnapshot&, Snapshot);

UCLASS(BlueprintType, Blueprintable)
class PROJECT_EDEN_API UOpenAIRequester : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable, Category = "OpenAI")
        FOnScatterParams OnScatterParams;

    UPROPERTY(BlueprintAssignable, Category = "OpenAI")
        FOnPlayerEvaluationReady OnPlayerEvaluationReady;

    UFUNCTION(BlueprintCallable, Category = "OpenAI")
        void SendOpenAIRequest();

    UFUNCTION(BlueprintCallable, Category = "OpenAI")
        void SendPlayerEvaluationRequest();

private:
    bool TryCreateAuthorizedRequest(const FString& Payload, TSharedRef<IHttpRequest, ESPMode::ThreadSafe>& OutRequest) const;
    FString BuildScatterPayloadJSON() const;
    FString BuildEvaluationPayloadJSON() const;
};
