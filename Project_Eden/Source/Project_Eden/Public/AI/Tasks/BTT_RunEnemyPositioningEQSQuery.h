#pragma once

#include "BehaviorTree/BTTaskNode.h"
#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "BTT_RunEnemyPositioningEQSQuery.generated.h"

class UBlackboardComponent;
class UEnvQuery;
class UBehaviorTree;
struct FEnvQueryRequest;
struct FEnvQueryResult;

struct FEnemyPositioningEQSQueryTaskMemory
{
	// 비동기 EQS 요청 취소/완료 추적에 사용하는 Request ID다.
	int32 RequestID = INDEX_NONE;
};

UCLASS()
class PROJECT_EDEN_API UBTT_RunEnemyPositioningEQSQuery : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTT_RunEnemyPositioningEQSQuery();

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;
	virtual void CleanupMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryClear::Type CleanupType) const override;
	virtual FString GetStaticDescription() const override;

protected:
	// 에디터에서 공용 EQS 에셋을 할당한다.
	UPROPERTY(EditAnywhere, Category = "EQS")
	TObjectPtr<UEnvQuery> QueryTemplate;

	// 위치 선택이므로 단일 최고 점수 아이템만 쓰는 것이 기본이다.
	UPROPERTY(EditAnywhere, Category = "EQS")
	TEnumAsByte<EEnvQueryRunMode::Type> RunMode = EEnvQueryRunMode::SingleResult;

	// EQS 결과를 기록할 Blackboard Vector 키다. 기본값은 MoveToLocation을 권장한다.
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector MoveLocationKey;

	// EQS 실패 시 이전 이동 위치를 지울지 여부다.
	UPROPERTY(EditAnywhere, Category = "EQS")
	bool bClearMoveLocationOnFail = true;

	// 나중에 디자이너가 특정 쿼리에서 named param 사용을 끌 수 있게 남겨둔 토글이다.
	UPROPERTY(EditAnywhere, Category = "EQS")
	bool bInjectBlackboardParameters = true;

	FQueryFinishedSignature QueryFinishedDelegate;

	void ApplyNamedParams(FEnvQueryRequest& QueryRequest, const UBlackboardComponent* BlackboardComponent) const;
	void OnQueryFinished(TSharedPtr<FEnvQueryResult> Result);
};
