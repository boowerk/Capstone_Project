#include "AI/Debug/EnemyAIDebugUtils.h"

#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY(LogEnemyAI);

namespace EnemyAIDebugUtils
{
	FString DescribeActor(const AActor* Actor)
	{
		if (!IsValid(Actor))
		{
			return TEXT("None");
		}

		return FString::Printf(TEXT("%s[%s]"), *Actor->GetName(), *Actor->GetClass()->GetName());
	}

	FString DescribeEvaluation(const FEnemyLLMEvaluation& Evaluation)
	{
		return FString::Printf(
			TEXT("Mode=%s Agg=%.2f Range=%.0f Retreat=%.2f Chase=%.2f Cover=%.2f Focus=%s"),
			*Evaluation.GetEnemyModeBlackboardValue().ToString(),
			Evaluation.Aggression,
			Evaluation.PreferredRange,
			Evaluation.RetreatThreshold,
			Evaluation.ChasePersistence,
			Evaluation.CoverPreference,
			*Evaluation.GetFocusTargetRuleBlackboardValue().ToString());
	}

	FString DescribeStimulus(const FAIStimulus& Stimulus)
	{
		return FString::Printf(
			TEXT("%s Age=%.2f Strength=%.2f Stimulus=%s"),
			Stimulus.WasSuccessfullySensed() ? TEXT("Sensed") : TEXT("Lost"),
			Stimulus.GetAge(),
			Stimulus.Strength,
			*DescribeLocation(Stimulus.StimulusLocation));
	}

	FString DescribeLocation(const FVector& Location)
	{
		return FString::Printf(TEXT("(X=%.0f Y=%.0f Z=%.0f)"), Location.X, Location.Y, Location.Z);
	}
}
