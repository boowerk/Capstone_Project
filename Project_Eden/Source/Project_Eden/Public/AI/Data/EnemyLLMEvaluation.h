#pragma once

#include "CoreMinimal.h"
#include "EnemyLLMEvaluation.generated.h"

UENUM(BlueprintType)
enum class EEnemyMode : uint8
{
	Patrol UMETA(DisplayName = "Patrol"),
	Hold UMETA(DisplayName = "Hold"),
	Pressure UMETA(DisplayName = "Pressure"),
	Retreat UMETA(DisplayName = "Retreat")
};

UENUM(BlueprintType)
enum class EEnemyFocusTargetRule : uint8
{
	CurrentThreat UMETA(DisplayName = "Current Threat"),
	Nearest UMETA(DisplayName = "Nearest"),
	Weakest UMETA(DisplayName = "Weakest"),
	PlayerFirst UMETA(DisplayName = "Player First")
};

// FJsonObjectConverter로 받을 JSON 전용 임시 모델이다.
USTRUCT()
struct PROJECT_EDEN_API FEnemyLLMEvaluationJsonModel
{
	GENERATED_BODY()

	UPROPERTY()
	FString EnemyMode = TEXT("Patrol");

	UPROPERTY()
	float Aggression = 0.5f;

	UPROPERTY()
	float PreferredRange = 600.0f;

	UPROPERTY()
	float RetreatThreshold = 0.35f;

	UPROPERTY()
	float ChasePersistence = 0.5f;

	UPROPERTY()
	float CoverPreference = 0.35f;

	UPROPERTY()
	FString FocusTargetRule = TEXT("CurrentThreat");
};

USTRUCT(BlueprintType)
struct PROJECT_EDEN_API FEnemyLLMEvaluation
{
	GENERATED_BODY()

	// 공용 BT의 상위 분기를 결정할 고수준 모드다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|LLM")
	EEnemyMode EnemyMode = EEnemyMode::Patrol;

	// 0.0~1.0 범위의 공격 성향이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|LLM")
	float Aggression = 0.5f;

	// 전투 시 유지하고 싶어하는 선호 거리(cm)다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|LLM")
	float PreferredRange = 600.0f;

	// 체력 비율이 이 값 아래로 내려가면 후퇴 판단에 사용할 수 있다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|LLM")
	float RetreatThreshold = 0.35f;

	// 0.0~1.0 범위의 추적 집요함이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|LLM")
	float ChasePersistence = 0.5f;

	// 0.0~1.0 범위의 엄폐 선호도다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|LLM")
	float CoverPreference = 0.35f;

	// 타겟 우선순위 규칙이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|LLM")
	EEnemyFocusTargetRule FocusTargetRule = EEnemyFocusTargetRule::CurrentThreat;

	void ValidateAndClamp();

	static FEnemyLLMEvaluation MakeSafeDefault();

	// Blackboard Name 키에 바로 넣기 쉽도록 안전한 이름으로 변환한다.
	FName GetEnemyModeBlackboardValue() const;
	FName GetFocusTargetRuleBlackboardValue() const;
};

struct PROJECT_EDEN_API FEnemyLLMEvaluationParser
{
	// JSON 파싱에 실패하면 false를 반환하고, OutEvaluation에는 안전한 기본값을 넣는다.
	static bool ParseFromJson(const FString& JsonPayload, FEnemyLLMEvaluation& OutEvaluation);

	static EEnemyMode ParseEnemyMode(const FString& InValue, EEnemyMode DefaultValue = EEnemyMode::Patrol);
	static EEnemyFocusTargetRule ParseFocusTargetRule(const FString& InValue, EEnemyFocusTargetRule DefaultValue = EEnemyFocusTargetRule::CurrentThreat);

	static FName ToBlackboardName(EEnemyMode InValue);
	static FName ToBlackboardName(EEnemyFocusTargetRule InValue);
};
