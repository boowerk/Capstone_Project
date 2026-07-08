#pragma once

#include "BehaviorTree/BTTaskNode.h"
#include "CoreMinimal.h"
#include "BTT_ExecuteMatadorPattern.generated.h"

class UBehaviorTreeComponent;

UCLASS()
class PROJECT_EDEN_API UBTT_ExecuteMatadorPattern : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTT_ExecuteMatadorPattern();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

	UPROPERTY(EditAnywhere, Category = "AI|Matador")
	bool bStopMovementBeforePattern = true;

	UPROPERTY(EditAnywhere, Category = "AI|Matador")
	bool bFaceTargetBeforePattern = true;

	UPROPERTY(EditAnywhere, Category = "AI|Matador")
	bool bFallbackToPlayerPawn = true;

	UPROPERTY(EditAnywhere, Category = "AI|Matador")
	bool bEnterGroggyWhenChainComplete = true;

	UPROPERTY(EditAnywhere, Category = "AI|Matador")
	bool bSucceedWhenBullAlreadyActive = true;

	UPROPERTY(EditAnywhere, Category = "AI|Matador", meta = (ClampMin = "0.0", Units = "cm"))
	float BullPatternMinRange = 0.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Matador", meta = (ClampMin = "0.0", Units = "cm"))
	float BullPatternMaxRange = 5000.0f;
};
