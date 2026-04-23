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

	// Designers can widen this band if attacks flicker near PreferredRange.
	UPROPERTY(EditAnywhere, Category = "AI|Tactics", meta = (ClampMin = "0.0"))
	float MinAttackWindow = 175.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Tactics", meta = (ClampMin = "0.0"))
	float MaxAttackWindow = 325.0f;

	UPROPERTY(EditAnywhere, Category = "AI|Tactics", meta = (ClampMin = "0.0"))
	float AttackWindowFloor = 125.0f;

	// Cover-heavy enemies may reposition when they lose line of sight.
	UPROPERTY(EditAnywhere, Category = "AI|Tactics", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CoverRepositionThreshold = 0.55f;

	// ChasePersistence must pass this value before lost LOS turns into chase intent.
	UPROPERTY(EditAnywhere, Category = "AI|Tactics", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LostLineOfSightChaseThreshold = 0.35f;

private:
	void UpdateTactics(UBehaviorTreeComponent& OwnerComp) const;
};
