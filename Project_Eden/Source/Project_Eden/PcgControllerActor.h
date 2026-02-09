#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PcgDataTypes.h"
#include "PcgControllerActor.generated.h"

class UOpenAIRequester;

UCLASS()
class PROJECT_EDEN_API APcgControllerActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APcgControllerActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OpenAI")
	TObjectPtr<UOpenAIRequester> OpenAIRequester;

	UFUNCTION(BlueprintImplementableEvent, Category = "PCG")
	void ApplyPcgParameters(const FPCGScatterParams& Params);

public:	
	UFUNCTION()
	void HandleScatterParams(FPCGScatterParams Params);
};
