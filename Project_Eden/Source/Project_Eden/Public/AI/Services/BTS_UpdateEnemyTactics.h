#pragma once

#include "BehaviorTree/BTService.h"
#include "CoreMinimal.h"
#include "BTS_UpdateEnemyTactics.generated.h"

class UBehaviorTreeComponent;

UCLASS()
class PROJECT_EDEN_API UBTS_UpdateEnemyTactics : public UBTService
{
	GENERATED_BODY()

public:
	UBTS_UpdateEnemyTactics();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnSearchStart(FBehaviorTreeSearchData& SearchData) override;
	virtual FString GetStaticServiceDescription() const override;
	virtual bool ShouldDeferTacticalBranchSelection() const { return false; }

	// Designers can widen this band if attacks flicker near PreferredRange.
	UPROPERTY(EditAnywhere, Category = "AI|Tactics", meta = (ClampMin = "0.0"))
	float MinAttackWindow = 175.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Tactics", meta = (ClampMin = "0.0"))
	float MaxAttackWindow = 325.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Tactics", meta = (ClampMin = "0.0"))
	float AttackWindowFloor = 125.0f;

	// 공격 밴드에 들어온 뒤에는 이만큼 더 벗어나야 Chase/Reposition으로 전환한다.
	UPROPERTY(EditAnywhere, Category = "AI|Tactics", meta = (ClampMin = "0.0", Units = "cm"))
	float AttackRangeExitHysteresis = 100.0f;

	// Melee enemies should still attack when the player gets closer than the ideal range.
	UPROPERTY(EditAnywhere, Category = "AI|Tactics")
	bool bAllowAttacksInsidePreferredRange = true;

	// Cover-heavy enemies may reposition when they lose line of sight.
	UPROPERTY(EditAnywhere, Category = "AI|Tactics", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CoverRepositionThreshold = 0.55f;

	// ChasePersistence must pass this value before lost LOS turns into chase intent.
	UPROPERTY(EditAnywhere, Category = "AI|Tactics", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LostLineOfSightChaseThreshold = 0.35f;

	// If we have a valid target but no stronger tactical branch won, keep chasing instead of falling back to patrol.
	UPROPERTY(EditAnywhere, Category = "AI|Tactics")
	bool bFallbackToChaseWhenTargetExists = true;

	// Re-evaluate the BT after this service flips branch keys, so patrol MoveTo/Wait cannot trap combat.
	UPROPERTY(EditAnywhere, Category = "AI|Tactics")
	bool bRestartTreeOnTacticalStateChange = true;

	// When a chase pulls the enemy too far from its anchor, pause target selection and return home.
	UPROPERTY(EditAnywhere, Category = "AI|Leash")
	bool bEnableLeashReturnHome = true;

	UPROPERTY(EditAnywhere, Category = "AI|Leash", meta = (ClampMin = "0.0"))
	float MaxChaseDistanceFromHome = 2200.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Leash", meta = (ClampMin = "0.0"))
	float ReturnHomeAcceptanceRadius = 150.0f;

	// Re-engage only after returning inside this fraction of the outer leash, preventing chase/return oscillation at the boundary.
	UPROPERTY(EditAnywhere, Category = "AI|Leash", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ReturnHomeReengageDistanceRatio = 0.75f;

private:
	void UpdateTactics(UBehaviorTreeComponent& OwnerComp) const;
	void RestartTreeIfTacticalStateChanged(
		UBehaviorTreeComponent& OwnerComp,
		bool bPreviousShouldRetreat,
		bool bPreviousCanAttack,
		bool bPreviousShouldReposition,
		bool bPreviousShouldChase,
		bool bPreviousShouldReturnHome,
		bool bShouldRetreat,
		bool bCanAttack,
		bool bShouldReposition,
		bool bShouldChase,
		bool bShouldReturnHome) const;
};
