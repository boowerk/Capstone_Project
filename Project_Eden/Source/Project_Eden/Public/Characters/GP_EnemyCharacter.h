#pragma once

#include "CoreMinimal.h"
#include "Characters/GP_BaseCharacter.h"
#include "GP_EnemyCharacter.generated.h"

class UAbilitySystemComponent;
class UAttributeSet;
class UBehaviorTree;
class UBlackboardData;
class UDrawSphereComponent;
class UEnemyArchetypeData;
struct FDataTableRowHandle;
struct FEnemyArchetypeTuning;
struct FEnemyLLMEvaluation;

UCLASS()
class PROJECT_EDEN_API AGP_EnemyCharacter : public AGP_BaseCharacter
{
	GENERATED_BODY()

public:
	AGP_EnemyCharacter();
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UAttributeSet* GetAttributeSet() const override;

	UFUNCTION(BlueprintPure, Category = "Boss")
	bool IsBossEnemy() const { return bIsBossEnemy; }

	UFUNCTION(BlueprintPure, Category = "Boss")
	FText GetBossDisplayName() const;

	UFUNCTION(BlueprintPure, Category = "AI")
	FVector GetBehaviorAnchorLocation() const;

	UFUNCTION(BlueprintPure, Category = "AI|Ranges")
	float GetReturnHomeDistance() const { return FMath::Max(0.0f, ReturnHomeDistance); }

	UFUNCTION(BlueprintPure, Category = "AI|Ranges")
	float GetReturnHomeAcceptanceRadius() const { return FMath::Max(0.0f, ReturnHomeAcceptanceRadius); }

	UFUNCTION(BlueprintPure, Category = "AI|Ranges")
	float GetPatrolRadius() const { return FMath::Max(0.0f, PatrolRadius); }

	UFUNCTION(BlueprintPure, Category = "AI|Perception")
	float GetSightRadius() const { return FMath::Max(0.0f, SightRadius); }

	UFUNCTION(BlueprintPure, Category = "AI|Perception")
	float GetLoseSightRadius() const { return FMath::Max(GetSightRadius(), LoseSightRadius); }

	UFUNCTION(BlueprintPure, Category = "AI|Perception")
	float GetPeripheralVisionAngleDegrees() const { return FMath::Clamp(PeripheralVisionAngleDegrees, 0.0f, 180.0f); }

	UFUNCTION(BlueprintPure, Category = "AI")
	UBehaviorTree* GetBehaviorTreeAssetOverride() const { return BehaviorTreeAssetOverride; }

	UFUNCTION(BlueprintPure, Category = "AI")
	UBlackboardData* GetBlackboardAssetOverride() const { return BlackboardAssetOverride; }

	// 현재 적 인스턴스의 시작 성향값을 계산해 공유 Blackboard에 넣을 준비를 한다.
	bool BuildInitialEnemyEvaluation(FEnemyLLMEvaluation& OutEvaluation) const;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	// 향후 EQS나 복귀 로직에서 사용할 기준 위치를 월드에 배치할 수 있도록 유지한다.
	UPROPERTY(EditInstanceOnly, Category = "AI", meta = (MakeEditWidget = "true"))
	FVector BehaviorAnchorOffset = FVector::ZeroVector;

	// Designers tune these on the placed enemy; BT, EQS, perception, and editor debug all read the same values.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Ranges", meta = (ClampMin = "0.0", Units = "cm"))
	float ReturnHomeDistance = 2200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Ranges", meta = (ClampMin = "0.0", Units = "cm"))
	float ReturnHomeAcceptanceRadius = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Ranges", meta = (ClampMin = "0.0", Units = "cm"))
	float PatrolRadius = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Perception", meta = (ClampMin = "0.0", Units = "cm"))
	float SightRadius = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Perception", meta = (ClampMin = "0.0", Units = "cm"))
	float LoseSightRadius = 2400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Perception", meta = (ClampMin = "0.0", ClampMax = "180.0", Units = "deg"))
	float PeripheralVisionAngleDegrees = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Debug")
	bool bShowAIRangesInEditor = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Debug", meta = (EditCondition = "bShowAIRangesInEditor"))
	bool bDrawAIRangesOnlyWhenSelected = true;

	// 테스트 중에는 적 액터 자체에서 공유 BT/Blackboard를 직접 지정할 수 있게 노출한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Assets")
	TObjectPtr<UBehaviorTree> BehaviorTreeAssetOverride;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Assets")
	TObjectPtr<UBlackboardData> BlackboardAssetOverride;

	// 가장 권장하는 경로로, 적 아키타입별 기본 성향을 DataAsset으로 정의한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Config")
	TObjectPtr<UEnemyArchetypeData> EnemyArchetypeData;

	// 필요하면 DataTable RowHandle로도 같은 구조를 읽을 수 있게 열어 둔다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Config", meta = (RowType = "/Script/Project_Eden.EnemyArchetypeTableRow"))
	FDataTableRowHandle EnemyArchetypeRow;

	// 디버깅이나 고정 개성 연출이 필요할 때 랜덤 시드를 수동 고정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Config")
	bool bOverridePersonalitySeed = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Config", meta = (EditCondition = "bOverridePersonalitySeed"))
	int32 PersonalitySeedOverride = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss")
	bool bIsBossEnemy = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss", meta = (EditCondition = "bIsBossEnemy"))
	FText BossDisplayName;

private:
#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = "AI|Debug", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UDrawSphereComponent> ReturnHomeRangeVisualizer;

	UPROPERTY(VisibleAnywhere, Category = "AI|Debug", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UDrawSphereComponent> PatrolRangeVisualizer;

	UPROPERTY(VisibleAnywhere, Category = "AI|Debug", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UDrawSphereComponent> SightRangeVisualizer;
#endif

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UAttributeSet> AttributeSet;

	FVector BehaviorAnchorLocation = FVector::ZeroVector;
	bool bHasBehaviorAnchorLocation = false;

	const FEnemyArchetypeTuning* ResolveEnemyArchetypeTuning() const;
	int32 ResolvePersonalitySeed() const;
	void RefreshAIRangeVisualizers();
};
