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
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sprinting);	// 달리기
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dash);		// 대시
		}
		namespace Skill
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(SkillRoot); // 부모 태그용
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Selection); // 스킬 선택/조준 상태 어빌리티
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Primary); // 평타
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot01);  // 스킬 1
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot02);  // 스킬 2
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ultimate); // 궁극기
			namespace Id
			{
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(WaterPuddle);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(NetTestProjectile);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(SplitShot);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(GroundBurst);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(ThrownBurst);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(PulseBurst);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(LineShock);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(MineBurst);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConeSlash);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(MagmaShot);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(GelmirFury);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(DarkSoloProjectile);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(DarkStone);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(DarkMist);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(LightningStrike);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(IceMist);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(CrystalTorrent);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(ShatteringCrystal);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(BigHammer);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(BigSword);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(MagicBigBubbles);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(MagicBubbles);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Potion);
			}
			namespace Visual
			{
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Projectile);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Impact);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cast);
			}
		}
		namespace Enemy
		{
			// 공격 계열
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack_Melee);   // 근접 공격
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack_Ranged);  // 원거리 공격 
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack_AoE);     // 광역 공격
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack_BossHeavy); // 보스 강공격
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack_BossArea);  // 보스 광역 공격
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack_BossSweep); // 보스 전방 휩쓸기 공격
            
			// 유틸리티 및 특수 계열
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Utility_Dash);   // 이동기 돌진기 등
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Utility_Summon); // 쫄몹 소환
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Utility_Buff);   // 자버프
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Utility_BossSummon);     // 보스 소환 패턴
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Utility_BossPhaseShift); // 보스 페이즈 전환 패턴
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Utility_MatadorBullPattern); // Matador bull decoy charge pattern
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Utility_MatadorGroggy);      // Matador groggy/recover state ability
            
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
		{													// 속성			영어	모델링 수
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Pyros);			// 화염 속성		Pyros	1
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Hydro);			// 물 속성		Hydro	1/2
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Volt);			// 전격 속성		Volt	0
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Aero);			// 바람 속성		Aero	1
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Lux);			// 빛 속성		Lux		0
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Chaos);			// 혼돈 속성		Chaos	0
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Brute);			// 물리 속성		Brute	0
		}
		namespace Coef
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Atk);			// 공격력 계수
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Def);			// 대상 방어력 계수
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Hp);				// 대상 최대 체력 계수
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(M_Atk);			// 마법력 계수
		}
		namespace Data
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Base);			// 물리/공통 기본 데미지
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(BaseSpell);		// 마법 기본 데미지
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(ToughnessBase);	// 강인도 기본 데미지
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Multiplier);
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
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(MatadorGuarded); // Matador guarded damage reduction state
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Groggy);         // Groggy removes guarded damage reduction
			}
		}
		namespace Movement
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sprinting);		// 달리기
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dash);		// 회피 대쉬
		}
		namespace Skill
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Selecting);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Previewing);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Projectile);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ray);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(TargetActor);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(GroundPosition);
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

	// [5] Tech : run-level player tech choices
	namespace Tech
	{
		namespace Element
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Pyros);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Hydro);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Volt);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Aero);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Lux);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Chaos);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Brute);
		}
	}

	// GameplayCue tags keep the engine-required "GameplayCue." root while staying in native project tag declarations.
	namespace GameplayCue
	{
		namespace Ability
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Trail_Magic);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Burst_Magic);
		}
	}

	// [6] Event : 일회성 이벤트 트리거
	namespace Event
	{
		namespace Player
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitReact);	// Player 피격 프레임
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(AttackHit);	// Player 공격 타격 프레임
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(ActionEnd);	// Player 액션 종료 프레임
			
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(ComboEnable);// Player 콤보 액션
		}
		namespace Skill
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConfirmPrimary);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConfirmSecondary);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cancel);
		}
		
		namespace Enemy
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitReact);	// Enemy 피격 프레임
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(AttackHit);	// Enemy 공격 타격 프레임
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(ActionEnd);	// Enemy 액션 종료 프레임
		}
	}

	// [7] Cooldown : 쿨다운
	namespace Cooldown
	{
		namespace Skill
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(WaterPuddle);
			
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(NetTestProjectile);

			UE_DECLARE_GAMEPLAY_TAG_EXTERN(SplitShot);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(MagmaShot);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(DarkSoloProjectile);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(CrystalTorrent);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(DarkStone);

			UE_DECLARE_GAMEPLAY_TAG_EXTERN(GroundBurst);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(LightningStrike);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(IceMist);

			UE_DECLARE_GAMEPLAY_TAG_EXTERN(ThrownBurst);

			UE_DECLARE_GAMEPLAY_TAG_EXTERN(PulseBurst);

			UE_DECLARE_GAMEPLAY_TAG_EXTERN(LineShock);

			UE_DECLARE_GAMEPLAY_TAG_EXTERN(MineBurst);

			UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConeSlash);

			UE_DECLARE_GAMEPLAY_TAG_EXTERN(DashSlash);

			UE_DECLARE_GAMEPLAY_TAG_EXTERN(LifeDrainTarget);
		}
		namespace Data
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Duration);
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
