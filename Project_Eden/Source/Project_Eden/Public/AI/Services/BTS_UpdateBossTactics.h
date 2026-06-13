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

	// 보스 체력이 이 비율 이하가 되면 2페이즈 패턴을 연다.
	UPROPERTY(EditAnywhere, Category = "AI|Boss", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PhaseTwoHealthRatio = 0.66f;

	// 보스 체력이 이 비율 이하가 되면 3페이즈 패턴과 소환 패턴을 연다.
	UPROPERTY(EditAnywhere, Category = "AI|Boss", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PhaseThreeHealthRatio = 0.33f;

	// 근접 거리 안에서는 보스 공격 분기가 유지되도록 한다.
	UPROPERTY(EditAnywhere, Category = "AI|Boss", meta = (ClampMin = "0.0", Units = "cm"))
	float HeavyAttackRange = 650.0f;

	// 2페이즈 이후, 시야가 있고 이 거리 안이면 광역 공격 후보가 된다.
	UPROPERTY(EditAnywhere, Category = "AI|Boss", meta = (ClampMin = "0.0", Units = "cm"))
	float AreaAttackRange = 1400.0f;

	// 이 거리 안에서 공격할 수 있으면 공격 태스크가 기본 공격/휘둘러치기를 랜덤 선택한다.
	UPROPERTY(EditAnywhere, Category = "AI|Boss", meta = (ClampMin = "0.0", Units = "cm"))
	float SweepAttackRange = 900.0f;

	// 광역 공격은 랜덤 기본/휘둘러치기보다 우선하는 별도 패턴으로 유지한다.
	UPROPERTY(EditAnywhere, Category = "AI|Boss", meta = (ClampMin = "0.1", Units = "s"))
	float AreaAttackInterval = 8.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Boss", meta = (ClampMin = "0.0", Units = "s"))
	float AreaAttackWindow = 1.2f;

	// 3페이즈 이후 소환 패턴의 선택 주기와 창을 설정한다.
	UPROPERTY(EditAnywhere, Category = "AI|Boss", meta = (ClampMin = "0.1", Units = "s"))
	float SummonInterval = 18.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Boss", meta = (ClampMin = "0.0", Units = "s"))
	float SummonWindow = 1.5f;

	// Matador bull pattern opens on a rhythm like other boss specials, but only while the state component is not groggy.
	UPROPERTY(EditAnywhere, Category = "AI|Boss|Matador", meta = (ClampMin = "0.1", Units = "s"))
	float BullPatternInterval = 6.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Boss|Matador", meta = (ClampMin = "0.0", Units = "s"))
	float BullPatternWindow = 3.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Boss|Matador", meta = (ClampMin = "0.0", Units = "cm"))
	float BullPatternMinRange = 450.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Boss|Matador", meta = (ClampMin = "0.0", Units = "cm"))
	float BullPatternMaxRange = 2400.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Boss|Matador", meta = (ClampMin = "0.0", Units = "cm"))
	float PreferredHoverHeight = 650.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Boss|Matador", meta = (ClampMin = "0.0", Units = "cm"))
	float PreferredAirRange = 1100.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Boss|Matador", meta = (ClampMin = "0.0", Units = "cm"))
	float TeleportDistanceThreshold = 1800.0f;

	// Crystal Seraph prism windows are mirrored to Blackboard and then scored by the shared selector.
	UPROPERTY(EditAnywhere, Category = "AI|Boss|Crystal Seraph", meta = (ClampMin = "0.1", Units = "s"))
	float CrystalPrismPatternInterval = 16.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Boss|Crystal Seraph", meta = (ClampMin = "0.0", Units = "s"))
	float CrystalPrismPatternWindow = 3.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Boss|Crystal Seraph", meta = (ClampMin = "0.1", Units = "s"))
	float CrystalLaserPatternInterval = 10.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Boss|Crystal Seraph", meta = (ClampMin = "0.0", Units = "s"))
	float CrystalLaserPatternWindow = 3.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Boss|Crystal Seraph", meta = (ClampMin = "0.0", Units = "cm"))
	float CrystalLaserMinRange = 450.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Boss|Crystal Seraph", meta = (ClampMin = "0.0", Units = "cm"))
	float CrystalLaserMaxRange = 2200.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Boss|Crystal Seraph", meta = (ClampMin = "0.0", Units = "cm"))
	float CrystalPrismMinRange = 500.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Boss|Crystal Seraph", meta = (ClampMin = "0.0", Units = "cm"))
	float CrystalPrismMaxRange = 1800.0f;

private:
	void UpdateBossTactics(UBehaviorTreeComponent& OwnerComp) const;
};
