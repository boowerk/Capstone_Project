#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTT_ChooseRepositionLocation.generated.h"

UCLASS()
class PROJECT_EDEN_API UBTT_ChooseRepositionLocation : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTT_ChooseRepositionLocation();

protected:
	// 너무 낮은 선호 거리가 들어와도 지나치게 붙지 않도록 하한을 둔다.
	UPROPERTY(EditAnywhere, Category = "AI")
	float MinPreferredRange = 250.0f;

	// 엄폐 선호도가 낮을 때 적용할 최소 측면 이동 거리다.
	UPROPERTY(EditAnywhere, Category = "AI")
	float MinLateralOffset = 150.0f;

	// 엄폐 선호도가 높을 때 적용할 최대 측면 이동 거리다.
	UPROPERTY(EditAnywhere, Category = "AI")
	float MaxLateralOffset = 500.0f;

	// 목표 위치 근처에서 네비게이션 샘플링할 반경이다.
	UPROPERTY(EditAnywhere, Category = "AI")
	float SearchRadius = 250.0f;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;
};
