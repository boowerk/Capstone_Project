#pragma once

#include "AI/Services/BTS_UpdateEnemyTactics.h"
#include "CoreMinimal.h"
#include "BTS_UpdateBossTactics.generated.h"

class UBehaviorTreeComponent;

UCLASS()
class PROJECT_EDEN_API UBTS_UpdateBossTactics : public UBTS_UpdateEnemyTactics
{
	GENERATED_BODY()

public:
	UBTS_UpdateBossTactics();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnSearchStart(FBehaviorTreeSearchData& SearchData) override;
	virtual FString GetStaticServiceDescription() const override;

	// 보스 체력이 이 비율 이하가 되면 2페이즈 패턴을 열어준다.
	UPROPERTY(EditAnywhere, Category = "AI|Boss", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PhaseTwoHealthRatio = 0.66f;

	// 보스 체력이 이 비율 이하가 되면 3페이즈 패턴과 소환 패턴을 열어준다.
	UPROPERTY(EditAnywhere, Category = "AI|Boss", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PhaseThreeHealthRatio = 0.33f;

	// 근접 공격 가능 상태에서 이 거리 안이면 강공격 후보가 된다.
	UPROPERTY(EditAnywhere, Category = "AI|Boss", meta = (ClampMin = "0.0", Units = "cm"))
	float HeavyAttackRange = 650.0f;

	// 2페이즈 이후, 시야가 있고 이 거리 안이면 광역 공격 후보가 된다.
	UPROPERTY(EditAnywhere, Category = "AI|Boss", meta = (ClampMin = "0.0", Units = "cm"))
	float AreaAttackRange = 1400.0f;

	// 광역 공격은 Ability 쿨다운과 별개로 BT 선택 빈도를 제한한다.
	UPROPERTY(EditAnywhere, Category = "AI|Boss", meta = (ClampMin = "0.1", Units = "s"))
	float AreaAttackInterval = 8.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Boss", meta = (ClampMin = "0.0", Units = "s"))
	float AreaAttackWindow = 1.2f;

	// 3페이즈 소환 패턴도 주기적 선택 창을 둬서 매 틱 반복 선택을 피한다.
	UPROPERTY(EditAnywhere, Category = "AI|Boss", meta = (ClampMin = "0.1", Units = "s"))
	float SummonInterval = 18.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Boss", meta = (ClampMin = "0.0", Units = "s"))
	float SummonWindow = 1.5f;

private:
	void UpdateBossTactics(UBehaviorTreeComponent& OwnerComp) const;
};
