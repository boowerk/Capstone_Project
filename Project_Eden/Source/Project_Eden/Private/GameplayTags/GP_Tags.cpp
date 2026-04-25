#include "GameplayTags/GP_Tags.h"

namespace GPTags
{
	namespace GPAbilities
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(ActivateOnGiven, "GPTags.GPAbilities.ActivateOnGiven", "Tag for abilities that auto-activate when granted");

		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Primary, "GPTags.GPAbilities.Primary", "Tag for the primary ability");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Targeting, "GPTags.GPAbilities.Targeting", "Tag for the targeting ability");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Skill, "GPTags.GPAbilities.Skill", "Tag for the skill ability");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ultimate, "GPTags.GPAbilities.Ultimate", "Tag for the ultimate ability");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Skill_Q, "GPTags.GPAbilities.Skill_Q", "Tag for the Q skill ability");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Skill_E, "GPTags.GPAbilities.Skill_E", "Tag for the E skill ability");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Skill_R, "GPTags.GPAbilities.Skill_R", "Tag for the R skill ability");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dash, "GPTags.GPAbilities.Dash", "Tag for the dash ability");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Rolling, "GPTags.GPAbilities.Rolling", "Tag for the rolling ability");

		namespace Enemy
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attack, "GPTags.GPAbilities.Enemy.Attack", "Tag for the enemy attack ability");
		}
	}
	
	namespace Status
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Fixed, "GPTags.Status.Fixed", "Tag for blocking movement and rotation");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Invincible, "GPTags.Status.Invincible", "Tag for invincibility frames");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Unstoppable, "GPTags.Status.Unstoppable", "Tag for ignoring hit reactions");
	}

	namespace Events
	{
		namespace Enemy
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(HitReact, "GPTags.Events.Enemy.HitReact", "Tag for enemy hit reaction");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(AttackHit, "GPTags.Events.Enemy.AttackHit", "Tag for enemy attack hit timing");
		}
	}

	namespace Cooldown
	{
		namespace Skill
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(WaterPuddle, "GPTags.Cooldown.Skill.WaterPuddle", "Tag for water puddle skill cooldown");
		}
	}
}
