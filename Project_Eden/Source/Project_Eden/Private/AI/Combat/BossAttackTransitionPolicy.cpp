#include "AI/Combat/BossAttackTransitionPolicy.h"

#include "GameplayTags/GP_Tags.h"

namespace BossAttackTransitionPolicy
{
	float ResolvePostAbilityCommitSeconds(
		const FGameplayTag& PatternTag,
		float DefaultCommitSeconds)
	{
		// Dark Knight GAS abilities delegate to wind-up and impact timers, so the
		// BT commit covers the last authored hit rather than only activation.
		if (PatternTag.MatchesTagExact(GPTags::Ability::Boss::DarkKnight::Basic))
		{
			return 1.9f;
		}
		if (PatternTag.MatchesTagExact(GPTags::Ability::Boss::DarkKnight::Heavy))
		{
			return 2.1f;
		}
		if (PatternTag.MatchesTagExact(GPTags::Ability::Boss::DarkKnight::Charge))
		{
			return 2.6f;
		}
		if (PatternTag.MatchesTagExact(GPTags::Ability::Boss::DarkKnight::DarkWave))
		{
			return 2.1f;
		}
		if (PatternTag.MatchesTagExact(GPTags::Ability::Boss::DarkKnight::GroundCrack))
		{
			return 3.5f;
		}

		// Crystal patterns spawn coordinators after their GAS telegraph finishes.
		if (PatternTag.MatchesTagExact(GPTags::Ability::Boss::CrystalSeraph::Laser))
		{
			return 3.7f;
		}
		if (PatternTag.MatchesTagExact(GPTags::Ability::Boss::CrystalSeraph::Area))
		{
			return 1.5f;
		}
		if (PatternTag.MatchesTagExact(GPTags::Ability::Boss::CrystalSeraph::Basic)
			|| PatternTag.MatchesTagExact(GPTags::Ability::Boss::CrystalSeraph::Prism))
		{
			return 0.6f;
		}
		if (PatternTag.MatchesTagExact(GPTags::Ability::Boss::CrystalSeraph::Teleport))
		{
			return 0.35f;
		}

		if (PatternTag.MatchesTagExact(GPTags::Ability::Enemy::Utility_MatadorBullPattern))
		{
			// The task also watches the live bull state and can finish earlier.
			return 6.2f;
		}
		if (PatternTag.MatchesTagExact(GPTags::Ability::Enemy::Utility_BossSummon))
		{
			return 0.5f;
		}

		// Long-lived GAS abilities already expose their exact lifecycle; their
		// task falls through to the short designer-configured recovery only.
		return FMath::Max(0.0f, DefaultCommitSeconds);
	}

	bool ShouldCompleteExternalAction(
		bool bExternalActionActive,
		float TotalElapsedSeconds,
		float ExternalActionTimeoutSeconds)
	{
		// 정상 신호 해제는 즉시 완료하고, 살아 있는 신호는 일반 GAS timeout과 분리된 상한까지 존중한다.
		return !bExternalActionActive
			|| FMath::Max(0.0f, TotalElapsedSeconds) >= FMath::Max(0.1f, ExternalActionTimeoutSeconds);
	}

	bool CanRequestDarkKnightPattern(
		bool bGuardBroken,
		bool bMeleeReady,
		bool bChargeReady,
		bool bDarkWaveReady,
		bool bGroundCrackReady)
	{
		// Guard break enters the groggy pattern through the same selector, while
		// every attack category must independently be able to open the branch.
		return bGuardBroken
			|| bMeleeReady
			|| bChargeReady
			|| bDarkWaveReady
			|| bGroundCrackReady;
	}
}
