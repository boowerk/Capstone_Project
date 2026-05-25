#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTagContainer.h"
#include "BTT_ExecuteEnemyAttack.generated.h"

UCLASS()
class PROJECT_EDEN_API UBTT_ExecuteEnemyAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTT_ExecuteEnemyAttack();

protected:
	// 공유 BT는 이 태그를 가진 적 공격 어빌리티를 실행한다.
	UPROPERTY(EditAnywhere, Category = "AI")
	FGameplayTag AttackAbilityTag;

	// 공격 직전에 이동을 멈춰 과도한 슬라이딩을 줄인다.
	UPROPERTY(EditAnywhere, Category = "AI")
	bool bStopMovementBeforeAttack = true;

	// 공격 직전에 현재 타겟을 바라보도록 회전시킨다.
	UPROPERTY(EditAnywhere, Category = "AI")
	bool bFaceTargetBeforeAttack = true;

	// 보스가 휘둘러치기 가능 상태일 때 기본 공격 대신 휘둘러치기를 선택할 확률입니다.
	UPROPERTY(EditAnywhere, Category = "AI|Boss", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BossSweepAttackChance = 0.5f;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;
};
