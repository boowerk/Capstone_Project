#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

// 프로젝트 전반에서 재사용할 네이티브 게임플레이 태그 모음
namespace GPTags
{
	// [0] Game : 시련 진행 상태 및 흐름 제어
	namespace Game
	{
		namespace Stage
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat);			// 전투 진행 중
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cleared);		// 시련 클리어됨
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Reward);			// 보상 선택 중
		}
	}
	
	// [1] Ability : 어빌리티 실행 및 식별용 태그 (기존 유지)
	namespace Ability
	{
		namespace System
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(ActivateOnGiven); // 패시브용
		}
		namespace Action
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Targeting); // 록온
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Interact);  // 상호작용
		}
		namespace Movement
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dash);      // 대시
		}
		namespace Skill
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Primary); // 평타
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot01);  // 스킬 1
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot02);  // 스킬 2
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ultimate); // 궁극기
		}
		namespace Enemy
		{
			// 공격 계열
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack_Melee);   // 근접 공격
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack_Ranged);  // 원거리 공격 
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack_AoE);     // 광역 공격
            
			// 유틸리티 및 특수 계열
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Utility_Dash);   // 이동기 돌진기 등
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Utility_Summon); // 쫄몹 소환
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Utility_Buff);   // 자버프
            
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Death);          // 뒤져요
		}
	}
	
	// [2] Damage : 데미지 타입 및 원소 속성 (RPG 필수)
	namespace Damage
	{
		namespace Type
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Physical);		// 물리 피해
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Magical);		// 마법 피해
		}
		namespace Element
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Fire);			// 화염 속성
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Water);			// 물 속성
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Lightning);		// 전격 속성
			
		}
	}

	// [3] State : 캐릭터 상태, 버프, 디버프
	namespace State
	{
		namespace Status
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Fixed);			// 이동/회전 불가
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Unstoppable);	// 저지불가 피격무시 데미지는 받음
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Invincible);		// 데미지 무적
			namespace Enemy
			{
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Aggroed);    // 어그로
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enraged);    // 광폭화
			}
		}
		namespace Movement
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sprinting);		// 달리기
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dash);		// 회피 대쉬
		}
		namespace Buff
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shield);			// 보호막
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Haste);			// 가속 (이속/공속 증가)
		}
		namespace Debuff
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Burn);			// 화상 (도트 뎀)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stun);			// 기절
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slow);			// 둔화
		}
	}

	// [4] Item : 유물 및 착용 장비 식별
	namespace Item
	{
		namespace Relic
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(BloodthirsterTest);		// 테스트용 피바라기
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(WarmogArmorTest);			// 테스트용 워모그
		}
		namespace Weapon
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(MeleeTest);		// 검 착용 중
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(RangeTest);		// 활 착용 중
		}
	}

	// [6] Event : 일회성 이벤트 트리거
	namespace Event
	{
		namespace Enemy
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitReact);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(AttackHit);
		}
	}

	// [7] Cooldown : 쿨다운
	namespace Cooldown
	{
		namespace Skill
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(WaterPuddle);
		}
	}
	
	// [8] AI : BT 연동 논리상태 사용할수도 있을것 같아서 추가
	namespace AI
	{
		namespace State
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Idle);      // 대기 중
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Patrol);    // 정찰 중
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Chasing);   // 추격 중
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat);    // 교전 중
		}
	}
}
