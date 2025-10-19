#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "OpenAIRequester.generated.h"

USTRUCT(BlueprintType)
struct PROJECT_EDEN_API FPCGScatterParams
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) float ScaleMin = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float ScaleMax = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float RotationMin = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float RotationMax = 0.0f;
};

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
