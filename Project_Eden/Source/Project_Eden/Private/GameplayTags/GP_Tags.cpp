#include "GameplayTags/GP_Tags.h" 


// 파일명만 일관성을 유지하기위해 _를 사용함 - 슝민
namespace GPTags
{
	namespace GPAbilities
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(ActivateOnGiven, "GPTags.GPAbilities.ActivateOnGiven", "Tag for Abilities");
		
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Primary, "GPTags.GPAbilities.Primary", "Tag for the Primary Ability");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Skill_Q, "GPTags.GPAbilities.Skill_Q", "Tag for the Skill_Q Ability");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Skill_E, "GPTags.GPAbilities.Skill_E", "Tag for the Skill_E Ability");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Skill_R, "GPTags.GPAbilities.Skill_R", "Tag for the Skill_R Ability");
		
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dash, "GPTags.GPAbilities.Dash", "Tag for the Dash Ability");
	}
	
	namespace Events
	{
		namespace Enemy
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(HitReact, "GPTags.Events.Enemy.HitReact", "Tag for the Enemy HitReact Event");
			
		}
	}
	
	namespace Cooldown
	{
		namespace Skill
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(WaterPuddle, "GPTags.Cooldown.Skill.WaterPuddle", "Tag for Water Puddle Skill Cooldown");
		}
	}
}
