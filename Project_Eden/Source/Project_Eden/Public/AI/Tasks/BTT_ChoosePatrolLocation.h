#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTT_ChoosePatrolLocation.generated.h"

UCLASS()
class PROJECT_EDEN_API UBTT_ChoosePatrolLocation : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTT_ChoosePatrolLocation();

protected:
	// 너무 좁은 반경만 반복 선택하지 않도록 최소 순찰 반경을 둔다.
	UPROPERTY(EditAnywhere, Category = "AI")
	float MinPatrolRadius = 300.0f;

	// 공용 적이 기본적으로 사용할 순찰 반경이다.
	UPROPERTY(EditAnywhere, Category = "AI")
	float BasePatrolRadius = 800.0f;

	// 선호 전투 거리가 긴 적일수록 조금 더 넓게 움직일 수 있도록 보정한다.
	UPROPERTY(EditAnywhere, Category = "AI")
	float PreferredRangeRadiusScale = 0.5f;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;
};
