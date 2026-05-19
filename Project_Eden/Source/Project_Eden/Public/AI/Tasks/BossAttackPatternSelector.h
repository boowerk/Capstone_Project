#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

struct FGPBossAttackPatternCandidate
{
	FGameplayTag AbilityTag;
	float Score = 0.0f;
	FName DebugName = NAME_None;
};

// Snapshot of the blackboard/evaluation state used to score the next boss attack pattern.
struct FGPBossAttackPatternContext
{
	FGameplayTag DefaultAttackAbilityTag;
	float DistanceToTarget = 0.0f;
	float Aggression = 0.5f;
	float PreferredRange = 600.0f;
	float HealthRatio = 1.0f;
	int32 BossPhase = 1;
	FName EnemyMode = TEXT("Patrol");
	FName FocusTargetRule = TEXT("CurrentThreat");
	bool bCanUseBossHeavyAttack = false;
	bool bCanUseBossSweepAttack = false;
	bool bCanUseBossAreaAttack = false;
	bool bCanSummonAdds = false;
};

// Shared deterministic selector used by the BT task and automation tests.
class PROJECT_EDEN_API FGPBossAttackPatternSelector
{
public:
	static TArray<FGPBossAttackPatternCandidate> BuildCandidates(const FGPBossAttackPatternContext& Context);
	static const FGPBossAttackPatternCandidate* SelectBestCandidate(const TArray<FGPBossAttackPatternCandidate>& Candidates);
	static FString DescribeCandidates(const TArray<FGPBossAttackPatternCandidate>& Candidates);

private:
	static void AddCandidate(TArray<FGPBossAttackPatternCandidate>& Candidates, const FGameplayTag& AbilityTag, float Score, FName DebugName);
};
