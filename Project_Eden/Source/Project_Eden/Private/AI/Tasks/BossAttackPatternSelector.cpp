#include "AI/Tasks/BossAttackPatternSelector.h"

#include "AI/Data/EnemyLLMEvaluation.h"
#include "GameplayTags/GP_Tags.h"

bool FGPBossAttackPatternRanges::IsWithinReach(float DistanceToTarget, float AttackReach)
{
	// Keep a small tolerance so capsule edge and animation root motion do not make valid hits flicker.
	constexpr float ReachTolerance = 25.0f;
	return FMath::Max(0.0f, DistanceToTarget) <= AttackReach + ReachTolerance;
}

bool FGPBossAttackPatternRanges::IsWithinBullPatternRange(float DistanceToTarget)
{
	const float SafeDistance = FMath::Max(0.0f, DistanceToTarget);
	return SafeDistance >= BullPatternMinRange && SafeDistance <= BullPatternMaxRange;
}

bool FGPBossAttackPatternRanges::IsWithinMatadorRapierRange(float DistanceToTarget)
{
	const float SafeDistance = FMath::Max(0.0f, DistanceToTarget);
	return SafeDistance >= MatadorRapierMinRange && SafeDistance <= MatadorRapierMaxRange;
}

bool FGPBossAttackPatternRanges::IsWithinCrystalLaserRange(float DistanceToTarget)
{
	const float SafeDistance = FMath::Max(0.0f, DistanceToTarget);
	return SafeDistance >= CrystalLaserMinRange && SafeDistance <= CrystalLaserMaxRange;
}

bool FGPBossAttackPatternRanges::IsWithinCrystalPrismRange(float DistanceToTarget)
{
	const float SafeDistance = FMath::Max(0.0f, DistanceToTarget);
	return SafeDistance >= CrystalPrismMinRange && SafeDistance <= CrystalPrismMaxRange;
}

void FGPBossAttackPatternSelector::AddCandidate(TArray<FGPBossAttackPatternCandidate>& Candidates, const FGameplayTag& AbilityTag, float Score, FName DebugName)
{
	if (!AbilityTag.IsValid() || Score <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	for (FGPBossAttackPatternCandidate& Candidate : Candidates)
	{
		if (Candidate.AbilityTag == AbilityTag)
		{
			Candidate.Score = FMath::Max(Candidate.Score, Score);
			Candidate.DebugName = DebugName;
			return;
		}
	}

	FGPBossAttackPatternCandidate Candidate;
	Candidate.AbilityTag = AbilityTag;
	Candidate.Score = Score;
	Candidate.DebugName = DebugName;
	Candidates.Add(Candidate);
}

TArray<FGPBossAttackPatternCandidate> FGPBossAttackPatternSelector::BuildCandidates(const FGPBossAttackPatternContext& Context)
{
	TArray<FGPBossAttackPatternCandidate> Candidates;

	const float DistanceToTarget = FMath::Max(0.0f, Context.DistanceToTarget);
	const float Aggression = FMath::Clamp(Context.Aggression, 0.0f, 1.0f);
	const float PreferredRange = FMath::Max(100.0f, Context.PreferredRange);
	const float HealthRatio = FMath::Clamp(Context.HealthRatio, 0.0f, 1.0f);
	const int32 BossPhase = FMath::Max(1, Context.BossPhase);
	const int32 ChainBreakTarget = FMath::Max(1, Context.ChainBreakTarget);
	const int32 ChainBreakCount = FMath::Clamp(Context.ChainBreakCount, 0, ChainBreakTarget);
	const float PreferredAirRange = FMath::Max(100.0f, Context.PreferredAirRange);
	const int32 WingCoreBreakTarget = FMath::Max(1, Context.WingCoreBreakTarget);
	const int32 WingCoreBreakCount = FMath::Clamp(Context.WingCoreBreakCount, 0, WingCoreBreakTarget);

	if (Context.bIsGroggy)
	{
		// During groggy the boss is intentionally open, so the selector should not start new attacks.
		return Candidates;
	}

	if (Context.bIsCrystalSeraph)
	{
		if (WingCoreBreakCount >= WingCoreBreakTarget)
		{
			AddCandidate(Candidates, GPTags::Ability::Boss::CrystalSeraph::Groggy, 5.0f, TEXT("CrystalSeraphGroggy"));
			return Candidates;
		}

		if (Context.bWingCoreExposed)
		{
			// While the core is exposed, stop normal pressure so the player clearly reads the damage window.
			return Candidates;
		}

		if (Context.bShouldTeleport)
		{
			// Crystal Seraph re-enters its authored air range before evaluating damage patterns.
			AddCandidate(Candidates, GPTags::Ability::Boss::CrystalSeraph::Teleport, 4.0f, TEXT("CrystalTeleport"));
			return Candidates;
		}

		const bool bCanUseCrystalLaser = Context.bCanUseLaserPattern && FGPBossAttackPatternRanges::IsWithinCrystalLaserRange(DistanceToTarget);
		const bool bCanUseCrystalPrism = Context.bCanUsePrismPattern && FGPBossAttackPatternRanges::IsWithinCrystalPrismRange(DistanceToTarget);
		const float CrystalPhaseBonus = FMath::Clamp(static_cast<float>(BossPhase - 1) * 0.2f, 0.0f, 0.45f);
		const float CrystalAirFit = 1.0f - FMath::Clamp(FMath::Abs(DistanceToTarget - PreferredAirRange) / PreferredAirRange, 0.0f, 1.0f);
		const float CorePressure = static_cast<float>(WingCoreBreakCount) / static_cast<float>(WingCoreBreakTarget);

		if (!Context.bCrystalPrismActive && bCanUseCrystalPrism)
		{
			const float PrismScore = 1.05f + CrystalAirFit * 0.3f + CrystalPhaseBonus + CorePressure * 0.15f;
			AddCandidate(Candidates, GPTags::Ability::Boss::CrystalSeraph::Prism, PrismScore, TEXT("CrystalPrism"));
		}

		if (Context.bCrystalPrismActive && bCanUseCrystalLaser)
		{
			const float LaserScore = 1.2f + CrystalPhaseBonus + CorePressure * 0.2f + (BossPhase >= 2 ? 0.2f : 0.0f);
			AddCandidate(Candidates, GPTags::Ability::Boss::CrystalSeraph::Laser, LaserScore, TEXT("SeraphLaser"));
		}

		if (Context.bCanUseBossAreaAttack && BossPhase >= 2 && FGPBossAttackPatternRanges::IsWithinReach(DistanceToTarget, FGPBossAttackPatternRanges::AreaAttackReach))
		{
			const float AreaScore = 0.7f + CrystalPhaseBonus + (BossPhase >= 3 ? 0.35f : 0.0f);
			AddCandidate(Candidates, GPTags::Ability::Boss::CrystalSeraph::Area, AreaScore, TEXT("CrystalSanctuary"));
		}

		AddCandidate(Candidates, GPTags::Ability::Boss::CrystalSeraph::Basic, 0.45f + CrystalAirFit * 0.2f, TEXT("CrystalShard"));

		Candidates.Sort([](const FGPBossAttackPatternCandidate& Left, const FGPBossAttackPatternCandidate& Right)
		{
			return Left.Score > Right.Score;
		});

		return Candidates;
	}

	if (Context.bIsDarkArmorKnight)
	{
		if (Context.bGuardBroken)
		{
			AddCandidate(Candidates, GPTags::Ability::Boss::DarkKnight::Groggy, 5.0f, TEXT("DarkKnightGroggy"));
			return Candidates;
		}
		const float PhasePressure = FMath::Clamp(static_cast<float>(BossPhase - 1) * 0.22f, 0.0f, 0.45f);
		const bool bInMeleeRange = DistanceToTarget <= FGPBossAttackPatternRanges::DarkKnightMeleeReach;
		const bool bInChargeRange = DistanceToTarget >= FGPBossAttackPatternRanges::DarkKnightChargeMinRange
			&& DistanceToTarget <= FGPBossAttackPatternRanges::DarkKnightChargeMaxRange;

		if (Context.bCanUseDarkKnightCharge && bInChargeRange)
		{
			AddCandidate(Candidates, GPTags::Ability::Boss::DarkKnight::Charge, 1.15f + PhasePressure, TEXT("DarkKnightCharge"));
		}
		if (Context.bCanUseDarkWave && DistanceToTarget <= FGPBossAttackPatternRanges::DarkKnightDarkWaveMaxRange)
		{
			const float RangeBonus = DistanceToTarget > FGPBossAttackPatternRanges::DarkKnightMeleeReach ? 0.35f : 0.0f;
			AddCandidate(Candidates, GPTags::Ability::Boss::DarkKnight::DarkWave, 0.72f + RangeBonus + PhasePressure, TEXT("DarkWave"));
		}
		if (Context.bCanUseGroundCrack && DistanceToTarget <= FGPBossAttackPatternRanges::DarkKnightGroundCrackMaxRange)
		{
			AddCandidate(Candidates, GPTags::Ability::Boss::DarkKnight::GroundCrack, 0.62f + PhasePressure + (HealthRatio <= 0.6f ? 0.3f : 0.0f), TEXT("GroundCrack"));
		}
		if (Context.bCanUseDarkKnightHeavy && bInMeleeRange)
		{
			AddCandidate(Candidates, GPTags::Ability::Boss::DarkKnight::Heavy, 0.78f + PhasePressure + (HealthRatio <= 0.6f ? 0.15f : 0.0f), TEXT("DarkHeavy"));
		}
		if (Context.bCanUseDarkKnightBasic && bInMeleeRange)
		{
			const float MeleeFit = 1.0f - FMath::Clamp(FMath::Abs(DistanceToTarget - Context.PreferredMeleeRange) / FMath::Max(100.0f, Context.PreferredMeleeRange), 0.0f, 1.0f);
			AddCandidate(Candidates, GPTags::Ability::Boss::DarkKnight::Basic, 0.58f + MeleeFit * 0.35f + PhasePressure, TEXT("DarkBasic"));
		}

		Candidates.Sort([](const FGPBossAttackPatternCandidate& Left, const FGPBossAttackPatternCandidate& Right)
		{
			return Left.Score > Right.Score;
		});
		return Candidates;
	}

	if (ChainBreakCount >= ChainBreakTarget)
	{
		AddCandidate(Candidates, GPTags::Ability::Enemy::Utility_MatadorGroggy, 5.0f, TEXT("MatadorGroggy"));
		return Candidates;
	}

	const bool bPressureMode = Context.EnemyMode == FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyMode::Pressure);
	const bool bHoldMode = Context.EnemyMode == FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyMode::Hold);
	const bool bRetreatMode = Context.EnemyMode == FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyMode::Retreat);
	const bool bWeakestFocus = Context.FocusTargetRule == FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyFocusTargetRule::Weakest);
	const bool bPlayerFirstFocus = Context.FocusTargetRule == FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyFocusTargetRule::PlayerFirst);

	const float PreferredRangeFit = 1.0f - FMath::Clamp(FMath::Abs(DistanceToTarget - PreferredRange) / PreferredRange, 0.0f, 1.0f);
	const float ClosePressure = 1.0f - FMath::Clamp(DistanceToTarget / PreferredRange, 0.0f, 1.0f);
	const float FarPressure = FMath::Clamp((DistanceToTarget - PreferredRange) / PreferredRange, 0.0f, 1.0f);
	const float PhaseBonus = FMath::Clamp(static_cast<float>(BossPhase - 1) * 0.15f, 0.0f, 0.35f);
	const float AirRangeFit = 1.0f - FMath::Clamp(FMath::Abs(DistanceToTarget - PreferredAirRange) / PreferredAirRange, 0.0f, 1.0f);

	if (Context.bCanUseBullPattern && !Context.bBullPatternActive && FGPBossAttackPatternRanges::IsWithinBullPatternRange(DistanceToTarget))
	{
		const float ChainPressure = static_cast<float>(ChainBreakCount) / static_cast<float>(ChainBreakTarget);
		const float BullScore = 0.95f
			+ PhaseBonus
			+ AirRangeFit * 0.35f
			+ ChainPressure * 0.25f
			+ (bPressureMode ? 0.12f : 0.0f)
			+ (Context.bShouldTeleport ? 0.1f : 0.0f);
		AddCandidate(Candidates, GPTags::Ability::Enemy::Utility_MatadorBullPattern, BullScore, TEXT("MatadorBull"));
	}

	if (Context.bSuppressGenericBossAttacks && !Context.bBullPatternActive)
	{
		if (FGPBossAttackPatternRanges::IsWithinReach(DistanceToTarget, FGPBossAttackPatternRanges::MatadorCapeGustReach))
		{
			const float CapeScore = 0.72f
				+ ClosePressure * 0.25f
				+ PhaseBonus
				+ (HealthRatio <= 0.35f ? 0.25f : 0.0f);
			AddCandidate(Candidates, GPTags::Ability::Boss::Matador::CapeGust, CapeScore, TEXT("MatadorCapeGust"));
		}

		if (FGPBossAttackPatternRanges::IsWithinMatadorRapierRange(DistanceToTarget))
		{
			const float RapierRangeFit = 1.0f - FMath::Clamp(FMath::Abs(DistanceToTarget - 700.0f) / 700.0f, 0.0f, 1.0f);
			const float RapierScore = 0.68f
				+ RapierRangeFit * 0.28f
				+ PhaseBonus
				+ (bPressureMode ? 0.08f : 0.0f);
			AddCandidate(Candidates, GPTags::Ability::Boss::Matador::RapierThrust, RapierScore, TEXT("MatadorRapier"));
		}
	}

	if (!Context.bSuppressGenericBossAttacks && FGPBossAttackPatternRanges::IsWithinReach(DistanceToTarget, FGPBossAttackPatternRanges::BasicAttackReach))
	{
		const float BasicScore = 0.35f
			+ (1.0f - Aggression) * 0.25f
			+ ClosePressure * 0.2f
			+ (bHoldMode ? 0.2f : 0.0f)
			+ (bWeakestFocus ? 0.1f : 0.0f)
			+ (Context.bCanUseBossHeavyAttack ? 0.25f : 0.0f)
			+ (HealthRatio <= 0.45f ? 0.1f : 0.0f);
		// Basic must remain a scored candidate only when the scratch hitbox can actually reach the target.
		AddCandidate(Candidates, Context.DefaultAttackAbilityTag, BasicScore, TEXT("Basic"));
	}

	if (!Context.bSuppressGenericBossAttacks && Context.bCanSummonAdds)
	{
		const float SummonScore = 0.65f
			+ PhaseBonus
			+ (bHoldMode ? 0.25f : 0.0f)
			+ (bRetreatMode ? 0.15f : 0.0f)
			+ FarPressure * 0.2f;
		// Summon competes in the same selector so live LLM mode changes can redirect the next attack without a BT rebuild.
		AddCandidate(Candidates, GPTags::Ability::Enemy::Utility_BossSummon, SummonScore, TEXT("Summon"));
	}

	if (!Context.bSuppressGenericBossAttacks && Context.bCanUseBossSweepAttack && FGPBossAttackPatternRanges::IsWithinReach(DistanceToTarget, FGPBossAttackPatternRanges::SweepAttackReach))
	{
		const float SweepScore = 0.2f
			+ Aggression * 0.25f
			+ PreferredRangeFit * 0.35f
			+ (bPressureMode ? 0.15f : 0.0f)
			+ (bPlayerFirstFocus ? 0.1f : 0.0f);
		AddCandidate(Candidates, GPTags::Ability::Enemy::Attack_BossSweep, SweepScore, TEXT("Sweep"));
	}

	if (!Context.bSuppressGenericBossAttacks && Context.bCanUseBossAreaAttack && FGPBossAttackPatternRanges::IsWithinReach(DistanceToTarget, FGPBossAttackPatternRanges::AreaAttackReach))
	{
		const float AreaScore = 0.18f
			+ PhaseBonus
			+ FarPressure * 0.25f
			+ (1.0f - Aggression) * 0.12f
			+ (bHoldMode || bRetreatMode ? 0.2f : 0.0f);
		AddCandidate(Candidates, GPTags::Ability::Enemy::Attack_BossArea, AreaScore, TEXT("Area"));
	}

	if (!Context.bSuppressGenericBossAttacks && Context.bCanUseBossGroundHands
		&& FGPBossAttackPatternRanges::IsWithinReach(DistanceToTarget, FGPBossAttackPatternRanges::GroundHandsReach))
	{
		// Ground hands is a high-commitment Sans pressure pattern; its ability cooldown rotates the next scored candidate afterward.
		const float GroundHandsScore = 0.58f
			+ PreferredRangeFit * 0.18f
			+ PhaseBonus
			+ (bPressureMode ? 0.08f : 0.0f)
			+ (bPlayerFirstFocus ? 0.08f : 0.0f);
		AddCandidate(Candidates, GPTags::Ability::Enemy::Attack_BossGroundHands, GroundHandsScore, TEXT("GroundHands"));
	}

	Candidates.Sort([](const FGPBossAttackPatternCandidate& Left, const FGPBossAttackPatternCandidate& Right)
	{
		return Left.Score > Right.Score;
	});

	return Candidates;
}

const FGPBossAttackPatternCandidate* FGPBossAttackPatternSelector::SelectBestCandidate(const TArray<FGPBossAttackPatternCandidate>& Candidates)
{
	return Candidates.Num() > 0 ? &Candidates[0] : nullptr;
}

FString FGPBossAttackPatternSelector::DescribeCandidates(const TArray<FGPBossAttackPatternCandidate>& Candidates)
{
	TArray<FString> Parts;
	for (const FGPBossAttackPatternCandidate& Candidate : Candidates)
	{
		Parts.Add(FString::Printf(TEXT("%s:%.2f:%s"), *Candidate.DebugName.ToString(), Candidate.Score, *Candidate.AbilityTag.ToString()));
	}

	return FString::Join(Parts, TEXT(", "));
}
