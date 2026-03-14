#include "GameplayTags/GP_Tags.h" 


// 파일명만 일관성을 유지하기위해 _를 사용함 - 슝민
namespace GPTags
{
	namespace GPAbilities
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(ActivateOnGiven, "GPTags.GPAbilities.ActivateOnGiven", "Tag for Abilities");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Primary, "GPTags.GPAbilities.Primary", "Tag for the Primary Ability");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Rolling, "GPTags.GPAbilities.Rolling", "Tag for the Rolling Ability");
	}
	
	namespace Events
	{
		namespace Enemy
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(HitReact, "GPTags.Events.Enemy.HitReact", "Tag for the Enemy HitReact Event");
			
		}
	}
}
