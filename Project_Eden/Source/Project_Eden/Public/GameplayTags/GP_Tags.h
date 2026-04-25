#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

// 프로젝트 전반에서 재사용할 네이티브 게임플레이 태그 모음이다.
namespace GPTags
{
	namespace GPAbilities
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(ActivateOnGiven);

		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Primary);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Targeting);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ultimate);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Q);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_E);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_R);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dash);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Rolling);

		namespace Enemy
		{
			// 공유 BT의 공격 태스크가 찾는 적 공격 어빌리티 태그다.
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack);
		}
	}
	
	namespace Status
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Fixed);			// 이동 불가 상태
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Unstoppable);	// 피격 무시 상태
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Invincible);		// 무적 상태
	}

	namespace Events
	{
		namespace Enemy
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitReact);

			// 적 공격 몽타주에서 타격 프레임을 전달할 때 사용하는 이벤트 태그다.
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(AttackHit);
		}
	}

	namespace Cooldown
	{
		namespace Skill
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(WaterPuddle);
		}
	}
}
