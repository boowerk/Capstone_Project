#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

// 파일명만 일관성을 유지하기위해 _를 사용함 - 슝민
namespace GPTags
{
	namespace GPAbilities
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(ActivateOnGiven);

		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Primary);    // 평타
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Targeting);  // 구 입력 Q/타게팅
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill);      // 구 입력 E/스킬
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ultimate);   // 구 입력 R/궁극기
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Q);    // 신 입력 Q
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_E);    // 신 입력 E
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_R);    // 신 입력 R
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dash);       // 대시/회피
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Rolling);    // 구르기
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
