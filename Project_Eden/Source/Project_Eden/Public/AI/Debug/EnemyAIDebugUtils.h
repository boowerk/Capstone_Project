#pragma once

#include "AI/Data/EnemyLLMEvaluation.h"
#include "CoreMinimal.h"
#include "Perception/AIPerceptionTypes.h"

class AActor;

DECLARE_LOG_CATEGORY_EXTERN(LogEnemyAI, Log, All);

namespace EnemyAIDebugUtils
{
	// 로그에서 액터를 짧고 읽기 쉽게 표시한다.
	PROJECT_EDEN_API FString DescribeActor(const AActor* Actor);

	// 평가값 전체를 한 줄 문자열로 정리한다.
	PROJECT_EDEN_API FString DescribeEvaluation(const FEnemyLLMEvaluation& Evaluation);

	// 감지 성공/상실과 자극 위치를 짧게 보여준다.
	PROJECT_EDEN_API FString DescribeStimulus(const FAIStimulus& Stimulus);

	// EQS 결과 위치를 로그에서 읽기 쉬운 형식으로 정리한다.
	PROJECT_EDEN_API FString DescribeLocation(const FVector& Location);
}
