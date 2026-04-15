#include "AI/Data/EnemyArchetypeData.h"

FEnemyLLMEvaluation FEnemyArchetypeTuning::BuildEvaluation(int32 PersonalitySeed) const
{
	FEnemyLLMEvaluation Evaluation = BaseEvaluation;

	if (Variation.bEnableRandomVariation)
	{
		FRandomStream RandomStream(PersonalitySeed + PersonalitySeedOffset);

		// 아키타입 정체성은 유지하면서도 개체별로 약간씩만 다르게 만든다.
		Evaluation.Aggression += RandomStream.FRandRange(-Variation.AggressionVariance, Variation.AggressionVariance);
		Evaluation.PreferredRange += RandomStream.FRandRange(-Variation.PreferredRangeVariance, Variation.PreferredRangeVariance);
		Evaluation.ChasePersistence += RandomStream.FRandRange(-Variation.ChasePersistenceVariance, Variation.ChasePersistenceVariance);
		Evaluation.RetreatThreshold += RandomStream.FRandRange(-Variation.RetreatThresholdVariance, Variation.RetreatThresholdVariance);
		Evaluation.CoverPreference += RandomStream.FRandRange(-Variation.CoverPreferenceVariance, Variation.CoverPreferenceVariance);
	}

	Evaluation.ValidateAndClamp();
	return Evaluation;
}
