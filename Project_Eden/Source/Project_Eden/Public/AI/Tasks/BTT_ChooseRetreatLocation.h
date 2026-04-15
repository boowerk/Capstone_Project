#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTT_ChooseRetreatLocation.generated.h"

UCLASS()
class PROJECT_EDEN_API UBTT_ChooseRetreatLocation : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTT_ChooseRetreatLocation();

protected:
	// 지나치게 짧은 후퇴를 막기 위한 최소 거리다.
	UPROPERTY(EditAnywhere, Category = "AI")
	float MinRetreatDistance = 600.0f;

	// 선호 전투 거리를 후퇴 거리 계산에 얼마나 반영할지 결정한다.
	UPROPERTY(EditAnywhere, Category = "AI")
	float PreferredRangeScale = 1.0f;

	// 엄폐 선호도가 높을수록 더 멀리 빠질 수 있도록 보정한다.
	UPROPERTY(EditAnywhere, Category = "AI")
	float CoverPreferenceDistanceBonus = 400.0f;

	// 목표 위치 근처에서 네비게이션 샘플링할 반경이다.
	UPROPERTY(EditAnywhere, Category = "AI")
	float SearchRadius = 250.0f;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;
};
