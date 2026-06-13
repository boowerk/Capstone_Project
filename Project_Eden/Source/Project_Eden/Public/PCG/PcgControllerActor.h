#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PCG/PcgDataTypes.h"
#include "PcgControllerActor.generated.h"

class UOpenAIRequester;
class APcgControllerActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGPOnPcgLayoutReady, APcgControllerActor*, Controller);

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Minimap")
	bool bAutoNotifyMinimapAfterApplyParameters = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Minimap", meta = (ClampMin = "0.0"))
	float AutoMinimapCaptureDelay = 0.5f;

	UFUNCTION(BlueprintImplementableEvent, Category = "PCG")
	void ApplyPcgParameters(const FPCGScatterParams& Params);

public:	
	UFUNCTION()
	void HandleScatterParams(FPCGScatterParams Params);

	// BP PCG 생성이 비동기로 끝나는 경우, 실제 스폰 완료 지점에서 이 함수를 호출하면 미니맵이 현재 배치를 다시 캡처합니다.
	UFUNCTION(BlueprintCallable, Category = "PCG|Minimap")
	void NotifyPcgGenerationFinished(float MinimapCaptureDelay = 0.25f);

	UPROPERTY(BlueprintAssignable, Category = "PCG|Minimap")
	FGPOnPcgLayoutReady OnPcgLayoutReady;
};
