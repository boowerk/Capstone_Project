#include "AI/Data/EnemyLLMEvaluation.h"

#include "AI/Debug/EnemyAIDebugUtils.h"
#include "JsonObjectConverter.h"

namespace
{
	FString MakeShortJsonPreview(const FString& JsonPayload)
	{
		constexpr int32 MaxPreviewLength = 160;
		const FString TrimmedPayload = JsonPayload.TrimStartAndEnd();
		return TrimmedPayload.Len() > MaxPreviewLength
			? FString::Printf(TEXT("%s..."), *TrimmedPayload.Left(MaxPreviewLength))
			: TrimmedPayload;
	}

	FString NormalizeEnumToken(const FString& InValue)
	{
		FString NormalizedValue;
		NormalizedValue.Reserve(InValue.Len());

		for (const TCHAR Character : InValue)
		{
			if (FChar::IsWhitespace(Character) || Character == TEXT('_') || Character == TEXT('-'))
			{
				continue;
			}

			NormalizedValue.AppendChar(FChar::ToLower(Character));
		}

		return NormalizedValue;
	}
}

void FEnemyLLMEvaluation::ValidateAndClamp()
{
	Aggression = FMath::Clamp(Aggression, 0.0f, 1.0f);
	PreferredRange = FMath::Clamp(PreferredRange, 0.0f, 3000.0f);
	RetreatThreshold = FMath::Clamp(RetreatThreshold, 0.0f, 1.0f);
	ChasePersistence = FMath::Clamp(ChasePersistence, 0.0f, 1.0f);
	CoverPreference = FMath::Clamp(CoverPreference, 0.0f, 1.0f);
}

FEnemyLLMEvaluation FEnemyLLMEvaluation::MakeSafeDefault()
{
	FEnemyLLMEvaluation DefaultEvaluation;
	DefaultEvaluation.ValidateAndClamp();
	return DefaultEvaluation;
}

FName FEnemyLLMEvaluation::GetEnemyModeBlackboardValue() const
{
	return FEnemyLLMEvaluationParser::ToBlackboardName(EnemyMode);
}

FName FEnemyLLMEvaluation::GetFocusTargetRuleBlackboardValue() const
{
	return FEnemyLLMEvaluationParser::ToBlackboardName(FocusTargetRule);
}

bool FEnemyLLMEvaluationParser::ParseFromJson(const FString& JsonPayload, FEnemyLLMEvaluation& OutEvaluation)
{
	OutEvaluation = FEnemyLLMEvaluation::MakeSafeDefault();

	if (JsonPayload.TrimStartAndEnd().IsEmpty())
	{
		// 빈 응답은 안전하게 실패 처리해 이전 AI 상태를 유지한다.
		UE_LOG(LogEnemyAI, Warning, TEXT("[JSON] 빈 평가 응답을 무시합니다."));
		return false;
	}

	FEnemyLLMEvaluationJsonModel JsonModel;
	const bool bParsed = FJsonObjectConverter::JsonObjectStringToUStruct<FEnemyLLMEvaluationJsonModel>(JsonPayload, &JsonModel, 0, 0);
	if (!bParsed)
	{
		UE_LOG(LogEnemyAI, Warning, TEXT("[JSON] 파싱 실패: %s"), *MakeShortJsonPreview(JsonPayload));
		return false;
	}

	OutEvaluation.EnemyMode = ParseEnemyMode(JsonModel.EnemyMode, OutEvaluation.EnemyMode);
	OutEvaluation.Aggression = JsonModel.Aggression;
	OutEvaluation.PreferredRange = JsonModel.PreferredRange;
	OutEvaluation.RetreatThreshold = JsonModel.RetreatThreshold;
	OutEvaluation.ChasePersistence = JsonModel.ChasePersistence;
	OutEvaluation.CoverPreference = JsonModel.CoverPreference;
	OutEvaluation.FocusTargetRule = ParseFocusTargetRule(JsonModel.FocusTargetRule, OutEvaluation.FocusTargetRule);
	OutEvaluation.ValidateAndClamp();

	UE_LOG(LogEnemyAI, Log, TEXT("[JSON] 파싱 성공: %s"), *EnemyAIDebugUtils::DescribeEvaluation(OutEvaluation));
	return true;
}

EEnemyMode FEnemyLLMEvaluationParser::ParseEnemyMode(const FString& InValue, EEnemyMode DefaultValue)
{
	const FString NormalizedValue = NormalizeEnumToken(InValue);

	if (NormalizedValue == TEXT("patrol") || NormalizedValue == TEXT("idle") || NormalizedValue == TEXT("default"))
	{
		return EEnemyMode::Patrol;
	}

	if (NormalizedValue == TEXT("hold") || NormalizedValue == TEXT("guard") || NormalizedValue == TEXT("defensive") || NormalizedValue == TEXT("defend"))
	{
		return EEnemyMode::Hold;
	}

	if (NormalizedValue == TEXT("pressure") || NormalizedValue == TEXT("aggressive") || NormalizedValue == TEXT("attack") || NormalizedValue == TEXT("engage"))
	{
		return EEnemyMode::Pressure;
	}

	if (NormalizedValue == TEXT("retreat") || NormalizedValue == TEXT("fallback") || NormalizedValue == TEXT("withdraw"))
	{
		return EEnemyMode::Retreat;
	}

	return DefaultValue;
}

EEnemyFocusTargetRule FEnemyLLMEvaluationParser::ParseFocusTargetRule(const FString& InValue, EEnemyFocusTargetRule DefaultValue)
{
	const FString NormalizedValue = NormalizeEnumToken(InValue);

	if (NormalizedValue == TEXT("currentthreat") || NormalizedValue == TEXT("threat") || NormalizedValue == TEXT("attacker"))
	{
		return EEnemyFocusTargetRule::CurrentThreat;
	}

	if (NormalizedValue == TEXT("nearest") || NormalizedValue == TEXT("closest"))
	{
		return EEnemyFocusTargetRule::Nearest;
	}

	if (NormalizedValue == TEXT("weakest") || NormalizedValue == TEXT("lowesthealth") || NormalizedValue == TEXT("lowhp"))
	{
		return EEnemyFocusTargetRule::Weakest;
	}

	if (NormalizedValue == TEXT("playerfirst") || NormalizedValue == TEXT("player") || NormalizedValue == TEXT("mainplayer"))
	{
		return EEnemyFocusTargetRule::PlayerFirst;
	}

	return DefaultValue;
}

FName FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyMode InValue)
{
	switch (InValue)
	{
	case EEnemyMode::Patrol:
		return TEXT("Patrol");

	case EEnemyMode::Hold:
		return TEXT("Hold");

	case EEnemyMode::Pressure:
		return TEXT("Pressure");

	case EEnemyMode::Retreat:
		return TEXT("Retreat");

	default:
		return TEXT("Patrol");
	}
}

FName FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyFocusTargetRule InValue)
{
	switch (InValue)
	{
	case EEnemyFocusTargetRule::CurrentThreat:
		return TEXT("CurrentThreat");

	case EEnemyFocusTargetRule::Nearest:
		return TEXT("Nearest");

	case EEnemyFocusTargetRule::Weakest:
		return TEXT("Weakest");

	case EEnemyFocusTargetRule::PlayerFirst:
		return TEXT("PlayerFirst");

	default:
		return TEXT("CurrentThreat");
	}
}
