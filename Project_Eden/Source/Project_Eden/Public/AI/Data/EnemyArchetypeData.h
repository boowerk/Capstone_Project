#pragma once

#include "CoreMinimal.h"
#include "AI/Data/EnemyLLMEvaluation.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "EnemyArchetypeData.generated.h"

USTRUCT(BlueprintType)
struct PROJECT_EDEN_API FEnemyPersonalityVariationSettings
{
	GENERATED_BODY()

	// 같은 아키타입 안에서도 개체별 미세한 성격 차이를 줄지 여부다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Variation")
	bool bEnableRandomVariation = false;

	// 0~1 스케일 계열 값의 랜덤 편차다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Variation", meta = (EditCondition = "bEnableRandomVariation", ClampMin = "0.0", ClampMax = "1.0"))
	float AggressionVariance = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Variation", meta = (EditCondition = "bEnableRandomVariation", ClampMin = "0.0", ClampMax = "1000.0"))
	float PreferredRangeVariance = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Variation", meta = (EditCondition = "bEnableRandomVariation", ClampMin = "0.0", ClampMax = "1.0"))
	float ChasePersistenceVariance = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Variation", meta = (EditCondition = "bEnableRandomVariation", ClampMin = "0.0", ClampMax = "1.0"))
	float RetreatThresholdVariance = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Variation", meta = (EditCondition = "bEnableRandomVariation", ClampMin = "0.0", ClampMax = "1.0"))
	float CoverPreferenceVariance = 0.1f;
};

USTRUCT(BlueprintType)
struct PROJECT_EDEN_API FEnemyArchetypeTuning
{
	GENERATED_BODY()

	// 공유 BT가 읽을 기본 성향값 세트다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Config")
	FEnemyLLMEvaluation BaseEvaluation;

	// 개체별 미세 변형 규칙이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Config")
	FEnemyPersonalityVariationSettings Variation;

	// 랜덤 스트림에 더해지는 고정 오프셋이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Config")
	int32 PersonalitySeedOffset = 0;

	FEnemyLLMEvaluation BuildEvaluation(int32 PersonalitySeed) const;
};

USTRUCT(BlueprintType)
struct PROJECT_EDEN_API FEnemyArchetypeTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Config")
	FEnemyArchetypeTuning Tuning;
};

UCLASS(BlueprintType)
class PROJECT_EDEN_API UEnemyArchetypeData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 에셋 하나로 적 아키타입의 기본 성향을 정의한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Config")
	FEnemyArchetypeTuning Tuning;
};
