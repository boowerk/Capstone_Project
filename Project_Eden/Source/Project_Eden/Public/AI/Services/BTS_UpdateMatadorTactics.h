#pragma once

#include "BehaviorTree/BTService.h"
#include "CoreMinimal.h"
#include "BTS_UpdateMatadorTactics.generated.h"

class UBehaviorTreeComponent;

UCLASS()
class PROJECT_EDEN_API UBTS_UpdateMatadorTactics : public UBTService
{
	GENERATED_BODY()

public:
	UBTS_UpdateMatadorTactics();

	// Both the service and the selector use this policy because the legacy
	// BB_EnemyCommon asset does not contain Matador-only readiness keys.
	bool IsBullPatternReady(
		bool bHasTarget,
		bool bReturningHome,
		bool bGroggy,
		bool bBullActive,
		float DistanceToTarget,
		bool bHasLineOfSight,
		float WorldTimeSeconds) const;

	// The legacy data-only common service calls this for Matador pawns so the
	// dedicated BT keeps its asset layout while receiving native boss tactics.
	void ApplyMatadorTactics(UBehaviorTreeComponent& OwnerComp) const;

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnSearchStart(FBehaviorTreeSearchData& SearchData) override;
	virtual FString GetStaticServiceDescription() const override;

	UPROPERTY(EditAnywhere, Category = "AI|Matador", meta = (ClampMin = "0.1", Units = "s"))
	float BullPatternInterval = 6.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Matador", meta = (ClampMin = "0.0", Units = "s"))
	float BullPatternWindow = 3.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Matador", meta = (ClampMin = "0.0", Units = "cm"))
	float BullPatternMinRange = 0.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Matador", meta = (ClampMin = "0.0", Units = "cm"))
	float BullPatternMaxRange = 5000.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Matador", meta = (ClampMin = "0.0", Units = "cm"))
	float PreferredAirRange = 1100.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Matador", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TooCloseRangeRatio = 0.75f;

	UPROPERTY(EditAnywhere, Category = "AI|Matador")
	bool bIgnoreLineOfSightForBullPattern = true;

	UPROPERTY(EditAnywhere, Category = "AI|Matador")
	bool bFallbackToPlayerPawn = true;

	UPROPERTY(EditAnywhere, Category = "AI|Matador")
	bool bEnterGroggyWhenChainComplete = true;

	UPROPERTY(EditAnywhere, Category = "AI|Matador")
	bool bForceRangeRepositionWhenTooClose = true;

};
