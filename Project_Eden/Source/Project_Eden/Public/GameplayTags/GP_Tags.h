#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

// 파일명만 일관성을 유지하기위해 _를 사용함 - 슝민
namespace GPTags
{
	namespace GPAbilities
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(ActivateOnGiven);
		
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Primary);   // 평타 (기존)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Q);
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_E);     // 스킬 (추가)
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_R);  // 궁극기 (추가)
        		
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dash);      // 대시/회피 (추가)
	}
	
	namespace Events
	{
		namespace Enemy
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitReact);
			
		}
	}
	
	namespace Cooldown
	{
		namespace Skill
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(WaterPuddle); // E스킬 물 장판 쿨타임
		}
	}
}

