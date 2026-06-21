#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

struct FGPBossAttackPatternCandidate
{
	FGameplayTag AbilityTag;
	float Score = 0.0f;
	FName DebugName = NAME_None;
};

struct PROJECT_EDEN_API FGPBossAttackPatternRanges
{
	static constexpr float BasicAttackReach = 360.0f;
	static constexpr float SweepAttackReach = 830.0f;
	static constexpr float AreaAttackReach = 850.0f;
	static constexpr float GroundHandsReach = 850.0f;
	static constexpr float BullPatternMinRange = 0.0f;
	static constexpr float BullPatternMaxRange = 5000.0f;
	static constexpr float MatadorRapierMinRange = 350.0f;
	static constexpr float MatadorRapierMaxRange = 1050.0f;
	static constexpr float MatadorCapeGustReach = 700.0f;
	static constexpr float CrystalLaserMinRange = 450.0f;
	static constexpr float CrystalLaserMaxRange = 2200.0f;
	static constexpr float CrystalPrismMinRange = 500.0f;
	static constexpr float CrystalPrismMaxRange = 1800.0f;
	static constexpr float DarkKnightMeleeReach = 420.0f;
	static constexpr float DarkKnightChargeMinRange = 900.0f;
	static constexpr float DarkKnightChargeMaxRange = 1800.0f;
	static constexpr float DarkKnightDarkWaveMaxRange = 2200.0f;
	static constexpr float DarkKnightGroundCrackMaxRange = 1800.0f;

	static bool IsWithinReach(float DistanceToTarget, float AttackReach);
	static bool IsWithinBullPatternRange(float DistanceToTarget);
	static bool IsWithinMatadorRapierRange(float DistanceToTarget);
	static bool IsWithinCrystalLaserRange(float DistanceToTarget);
	static bool IsWithinCrystalPrismRange(float DistanceToTarget);
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
	bool bCanUseBossGroundHands = false;
	bool bCanSummonAdds = false;
	int32 ChainBreakCount = 0;
	int32 ChainBreakTarget = 3;
	bool bIsGroggy = false;
	bool bCanUseBullPattern = false;
	bool bBullPatternActive = false;
	bool bSuppressGenericBossAttacks = false;
	bool bShouldTeleport = false;
	float PreferredHoverHeight = 650.0f;
	float PreferredAirRange = 1100.0f;
	bool bIsCrystalSeraph = false;
	bool bWingCoreExposed = false;
	bool bCanExposeWingCore = false;
	bool bCanUseLaserPattern = false;
	bool bCanUsePrismPattern = false;
	bool bCrystalPrismActive = false;
	int32 WingCoreBreakCount = 0;
	int32 WingCoreBreakTarget = 3;
	float TimeSinceLastLaser = 999.0f;
	float TimeSinceLastPrism = 999.0f;
	bool bIsDarkArmorKnight = false;
	bool bIsGuarding = false;
	bool bCanParry = false;
	bool bGuardBroken = false;
	bool bCanUseDarkKnightBasic = false;
	bool bCanUseDarkKnightHeavy = false;
	bool bCanUseDarkKnightSweep = false;
	bool bCanUseDarkKnightGuard = false;
	bool bCanUseDarkKnightCharge = false;
	bool bCanUseDarkWave = false;
	bool bCanUseGroundCrack = false;
	float GuardGauge = 100.0f;
	float MaxGuardGauge = 100.0f;
	float PreferredMeleeRange = 350.0f;
	FName LastHitDirection = TEXT("Front");
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
