#include "GameplayTags/GP_Tags.h"

namespace GPTags
{
    // [0] Game : 시련 진행 상태 및 흐름 제어
    namespace Game
    {
        namespace Stage
        {
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat, "GPTags.Game.Stage.Combat", "전투 진행 중");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cleared, "GPTags.Game.Stage.Cleared", "시련 클리어됨");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Reward, "GPTags.Game.Stage.Reward", "보상 선택 중");
        }
    }
    
    // [1] Ability : 어빌리티 실행 및 식별용 태그
    namespace Ability
    {
        namespace System
        {
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(ActivateOnGiven, "GPTags.Ability.System.ActivateOnGiven", "패시브용 어빌리티");
        }
        namespace Action
        {
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Targeting, "GPTags.Ability.Action.Targeting", "록온 동작");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Interact, "GPTags.Ability.Action.Interact", "상호작용 동작");
        }
        namespace Movement
        {
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Sprinting, "GPTags.Ability.Movement.Sprinting", "달리기 이동기");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dash, "GPTags.Ability.Movement.Dash", "대시 이동기");
        }
        namespace Skill
        {
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(SkillRoot, "GPTags.Ability.Skill", "스킬 전체 부모 태그");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Selection, "GPTags.Ability.Skill.Selection", "스킬 선택/조준 상태 어빌리티");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Primary, "GPTags.Ability.Skill.Primary", "평타");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot01, "GPTags.Ability.Skill.Slot01", "스킬 슬롯 1");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot02, "GPTags.Ability.Skill.Slot02", "스킬 슬롯 2");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ultimate, "GPTags.Ability.Skill.Ultimate", "궁극기");
            namespace Id
            {
                UE_DEFINE_GAMEPLAY_TAG_COMMENT(WaterPuddle, "GPTags.Ability.Skill.Id.WaterPuddle", "물웅덩이 스킬 식별 태그");
                UE_DEFINE_GAMEPLAY_TAG_COMMENT(NetTestProjectile, "GPTags.Ability.Skill.Id.NetTestProjectile", "테스트 프로젝타일 스킬 식별 태그");
                UE_DEFINE_GAMEPLAY_TAG_COMMENT(SplitShot, "GPTags.Ability.Skill.Id.SplitShot", "스플릿샷 스킬 식별 태그");
                UE_DEFINE_GAMEPLAY_TAG_COMMENT(GroundBurst, "GPTags.Ability.Skill.Id.GroundBurst", "그라운드 버스트 스킬 식별 태그");
                UE_DEFINE_GAMEPLAY_TAG_COMMENT(ThrownBurst, "GPTags.Ability.Skill.Id.ThrownBurst", "투척 버스트 스킬 식별 태그");
                UE_DEFINE_GAMEPLAY_TAG_COMMENT(PulseBurst, "GPTags.Ability.Skill.Id.PulseBurst", "펄스 버스트 스킬 식별 태그");
                UE_DEFINE_GAMEPLAY_TAG_COMMENT(LineShock, "GPTags.Ability.Skill.Id.LineShock", "라인 쇼크 스킬 식별 태그");
                UE_DEFINE_GAMEPLAY_TAG_COMMENT(MineBurst, "GPTags.Ability.Skill.Id.MineBurst", "마인 버스트 스킬 식별 태그");
                UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConeSlash, "GPTags.Ability.Skill.Id.ConeSlash", "콘 슬래시 스킬 식별 태그");
                UE_DEFINE_GAMEPLAY_TAG_COMMENT(MagmaShot, "GPTags.Ability.Skill.Id.MagmaShot", "마그마 샷 스킬 식별 태그");
                UE_DEFINE_GAMEPLAY_TAG_COMMENT(GelmirFury, "GPTags.Ability.Skill.Id.GelmirFury", "겔미어 퓨리 스킬 식별 태그");
                UE_DEFINE_GAMEPLAY_TAG_COMMENT(DarkSoloProjectile, "GPTags.Ability.Skill.Id.DarkSoloProjectile", "다크 단일 투사체 스킬 식별 태그");
                UE_DEFINE_GAMEPLAY_TAG_COMMENT(DarkStone, "GPTags.Ability.Skill.Id.DarkStone", "다크 스톤 스킬 식별 태그");
                UE_DEFINE_GAMEPLAY_TAG_COMMENT(DarkMist, "GPTags.Ability.Skill.Id.DarkMist", "다크 미스트 스킬 식별 태그");
                UE_DEFINE_GAMEPLAY_TAG_COMMENT(LightningStrike, "GPTags.Ability.Skill.Id.LightningStrike", "라이트닝 스트라이크 스킬 식별 태그");
                UE_DEFINE_GAMEPLAY_TAG_COMMENT(IceMist, "GPTags.Ability.Skill.Id.IceMist", "아이스 미스트 스킬 식별 태그");
                UE_DEFINE_GAMEPLAY_TAG_COMMENT(CrystalTorrent, "GPTags.Ability.Skill.Id.CrystalTorrent", "크리스탈 토렌트 스킬 식별 태그");
                UE_DEFINE_GAMEPLAY_TAG_COMMENT(ShatteringCrystal, "GPTags.Ability.Skill.Id.ShatteringCrystal", "섀터링 크리스탈 스킬 식별 태그");
                UE_DEFINE_GAMEPLAY_TAG_COMMENT(BigHammer, "GPTags.Ability.Skill.Id.BigHammer", "빅 해머 스킬 식별 태그");
                UE_DEFINE_GAMEPLAY_TAG_COMMENT(BigSword, "GPTags.Ability.Skill.Id.BigSword", "빅 소드 스킬 식별 태그");
                UE_DEFINE_GAMEPLAY_TAG_COMMENT(MagicBigBubbles, "GPTags.Ability.Skill.Id.MagicBigBubbles", "매직 빅 버블 스킬 식별 태그");
                UE_DEFINE_GAMEPLAY_TAG_COMMENT(MagicBubbles, "GPTags.Ability.Skill.Id.MagicBubbles", "매직 버블 스킬 식별 태그");
                UE_DEFINE_GAMEPLAY_TAG_COMMENT(Potion, "GPTags.Ability.Skill.Id.Potion", "포션 스킬 식별 태그");
            }
            namespace Visual
            {
                UE_DEFINE_GAMEPLAY_TAG_COMMENT(Projectile, "GPTags.Ability.Skill.Visual.Projectile", "투사체 비행 연출");
                UE_DEFINE_GAMEPLAY_TAG_COMMENT(Impact, "GPTags.Ability.Skill.Visual.Impact", "충돌 연출");
                UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cast, "GPTags.Ability.Skill.Visual.Cast", "시전 연출");
            }
        }
        namespace Enemy
        {
            // 공격 계열
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attack_Melee, "GPTags.Ability.Enemy.Attack_Melee", "적 근접 공격");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attack_Ranged, "GPTags.Ability.Enemy.Attack_Ranged", "적 원거리 공격");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attack_AoE, "GPTags.Ability.Enemy.Attack_AoE", "적 광역 공격");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attack_BossHeavy, "GPTags.Ability.Enemy.Attack_BossHeavy", "보스 강공격");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attack_BossArea, "GPTags.Ability.Enemy.Attack_BossArea", "보스 광역 공격");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attack_BossSweep, "GPTags.Ability.Enemy.Attack_BossSweep", "보스 전방 휩쓸기 공격");
            
            // 유틸리티 및 특수 계열
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Utility_Dash, "GPTags.Ability.Enemy.Utility_Dash", "적 이동/돌진기");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Utility_Summon, "GPTags.Ability.Enemy.Utility_Summon", "적 쫄몹 소환");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Utility_Buff, "GPTags.Ability.Enemy.Utility_Buff", "적 자가 버프");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Utility_BossSummon, "GPTags.Ability.Enemy.Utility_BossSummon", "보스 소환 패턴");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Utility_BossPhaseShift, "GPTags.Ability.Enemy.Utility_BossPhaseShift", "보스 페이즈 전환 패턴");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Utility_MatadorBullPattern, "GPTags.Ability.Enemy.Utility_MatadorBullPattern", "마타도르 황소 돌진 패턴");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Utility_MatadorGroggy, "GPTags.Ability.Enemy.Utility_MatadorGroggy", "마타도르 그로기/회복 패턴");
            
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Death, "GPTags.Ability.Enemy.Death", "적 사망 처리");
        }
    }
    
    // [2] Damage : 데미지 타입 및 원소 속성
    namespace Damage
    {
        namespace Type
        {
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Physical, "GPTags.Damage.Type.Physical", "물리 피해");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Magical, "GPTags.Damage.Type.Magical", "마법 피해");
        }
        namespace Element
        {
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Pyros, "GPTags.Damage.Element.Pyros", "화염 속성 (Pyros)");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Hydro, "GPTags.Damage.Element.Hydro", "물 속성 (Hydro)");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Volt, "GPTags.Damage.Element.Volt", "전격 속성 (Volt)");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Aero, "GPTags.Damage.Element.Aero", "바람 속성 (Aero)");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Lux, "GPTags.Damage.Element.Lux", "빛 속성 (Lux)");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Chaos, "GPTags.Damage.Element.Chaos", "혼돈 속성 (Chaos)");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Brute, "GPTags.Damage.Element.Brute", "물리/강타 속성 (Brute)");
        }
        namespace Coef
        {
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Atk, "Damage.Coef.Atk", "SetByCaller 공격력 계수");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Def, "Damage.Coef.Def", "SetByCaller 대상 방어력 계수");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Hp, "Damage.Coef.Hp", "SetByCaller 대상 최대 체력 계수");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(M_Atk, "Damage.Coef.M_Atk", "SetByCaller 마법력 계수");
        }
        namespace Data
        {
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Base, "Damage.Base", "SetByCaller 물리/공통 기본 데미지");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(BaseSpell, "Damage.BaseSpell", "SetByCaller 마법 기본 데미지");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(ToughnessBase, "Damage.ToughnessBase", "SetByCaller 강인도 기본 데미지");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Multiplier, "Damage.Multiplier", "SetByCaller 최종 스킬 피해 배율");
        }
    }

    // [3] State : 캐릭터 상태, 버프, 디버프
    namespace State
    {
        namespace Status
        {
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Fixed, "GPTags.State.Status.Fixed", "이동 및 회전 불가 상태");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Unstoppable, "GPTags.State.Status.Unstoppable", "저지 불가 (피격 경직 무시)");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Invincible, "GPTags.State.Status.Invincible", "데미지 무적 상태");
            
            namespace Enemy
            {
                UE_DEFINE_GAMEPLAY_TAG_COMMENT(Aggroed, "GPTags.State.Status.Enemy.Aggroed", "어그로 끌린 상태");
                UE_DEFINE_GAMEPLAY_TAG_COMMENT(Enraged, "GPTags.State.Status.Enemy.Enraged", "광폭화 상태");
                UE_DEFINE_GAMEPLAY_TAG_COMMENT(MatadorGuarded, "GPTags.State.Status.Enemy.MatadorGuarded", "마타도르 보스 기본 피해 감쇠 상태");
                UE_DEFINE_GAMEPLAY_TAG_COMMENT(Groggy, "GPTags.State.Status.Enemy.Groggy", "적 그로기 상태");
            }
        }
        namespace Movement
        {
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Sprinting, "GPTags.State.Movement.Sprinting", "달리기 상태");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dash, "GPTags.State.Movement.Dash", "대쉬 상태");
        }
        namespace Skill
        {
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Selecting, "GPTags.State.Skill.Selecting", "스킬 선택 입력 대기 상태");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Previewing, "GPTags.State.Skill.Previewing", "스킬 프리뷰 표시 상태");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Projectile, "GPTags.State.Skill.Projectile", "투사체 스킬 조준 상태");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ray, "GPTags.State.Skill.Ray", "레이/라인 스킬 조준 상태");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(TargetActor, "GPTags.State.Skill.TargetActor", "대상 선택 스킬 조준 상태");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(GroundPosition, "GPTags.State.Skill.GroundPosition", "지면 위치 선택 스킬 조준 상태");
        }
        namespace Buff
        {
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Shield, "GPTags.State.Buff.Shield", "보호막 버프");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Haste, "GPTags.State.Buff.Haste", "가속 버프 (이속/공속 증가)");
        }
        namespace Debuff
        {
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Burn, "GPTags.State.Debuff.Burn", "화상 디버프 (도트 뎀)");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Stun, "GPTags.State.Debuff.Stun", "기절 디버프");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slow, "GPTags.State.Debuff.Slow", "둔화 디버프");
        }
    }

    // [4] Item : 유물 및 착용 장비 식별
    namespace Item
    {
        namespace Relic
        {
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(BloodthirsterTest, "GPTags.Item.Relic.BloodthirsterTest", "테스트용 피바라기 유물");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(WarmogArmorTest, "GPTags.Item.Relic.WarmogArmorTest", "테스트용 워모그 유물");
        }
        namespace Weapon
        {
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(MeleeTest, "GPTags.Item.Weapon.MeleeTest", "검(근접) 착용 중");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(RangeTest, "GPTags.Item.Weapon.RangeTest", "활(원거리) 착용 중");
        }
    }

    // [5] Tech : run-level player tech choices
    namespace Tech
    {
        namespace Element
        {
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Pyros, "GPTags.Tech.Element.Pyros", "이번 판 화염 테크");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Hydro, "GPTags.Tech.Element.Hydro", "이번 판 물 테크");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Volt, "GPTags.Tech.Element.Volt", "이번 판 전격 테크");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Aero, "GPTags.Tech.Element.Aero", "이번 판 바람 테크");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Lux, "GPTags.Tech.Element.Lux", "이번 판 빛 테크");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Chaos, "GPTags.Tech.Element.Chaos", "이번 판 혼돈 테크");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Brute, "GPTags.Tech.Element.Brute", "이번 판 강타 테크");
        }
    }

    namespace GameplayCue
    {
        namespace Ability
        {
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Trail_Magic, "GameplayCue.Ability.Trail.Magic", "Ability magic trail VFX cue");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Burst_Magic, "GameplayCue.Ability.Burst.Magic", "Ability magic burst VFX cue");
        }
    }

    // [6] Event : 일회성 이벤트 트리거
    namespace Event
    {
        namespace Player
        {
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(HitReact, "GPTags.Event.Player.HitReact", "Player 피격 리액션 이벤트");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(AttackHit, "GPTags.Event.Player.AttackHit", "Player 공격 타격 판정 프레임 이벤트");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(ActionEnd, "GPTags.Event.Player.ActionEnd", "Player 액션 종료 프레임 이벤트");
            
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(ComboEnable, "GPTags.Event.Player.ComboEnable", "Player 콤보 액션 이벤트");
        }
        namespace Skill
        {
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConfirmPrimary, "GPTags.Event.Skill.Confirm.Primary", "선택 중인 스킬의 기본 확정 입력");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConfirmSecondary, "GPTags.Event.Skill.Confirm.Secondary", "선택 중인 스킬의 보조 확정 입력");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cancel, "GPTags.Event.Skill.Cancel", "선택 중인 스킬 취소 입력");
        }
        namespace Enemy
        {
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(HitReact, "GPTags.Event.Enemy.HitReact", "Enemy 피격 리액션 이벤트");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(AttackHit, "GPTags.Event.Enemy.AttackHit", "Enemy 공격 타격 판정 프레임 이벤트");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(ActionEnd, "GPTags.Event.Enemy.ActionEnd", "Enemy 액션 종료 프레임 이벤트");
        }
    }

    // [7] Cooldown : 쿨다운
    namespace Cooldown
    {
        namespace Skill
        {
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(WaterPuddle, "GPTags.Cooldown.Skill.WaterPuddle", "물웅덩이 스킬 쿨다운");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(NetTestProjectile, "GPTags.Cooldown.Skill.NetTestProjectile", "테스트 프로젝타일 스킬 쿨다운");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(SplitShot, "GPTags.Cooldown.Skill.SplitShot", "스플릿샷 스킬 쿨다운");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(MagmaShot, "GPTags.Cooldown.Skill.MagmaShot", "마그마 샷 스킬 쿨다운");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(DarkSoloProjectile, "GPTags.Cooldown.Skill.DarkSoloProjectile", "다크 단일 투사체 스킬 쿨다운");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(CrystalTorrent, "GPTags.Cooldown.Skill.CrystalTorrent", "크리스탈 토렌트 스킬 쿨다운");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(DarkStone, "GPTags.Cooldown.Skill.DarkStone", "다크 스톤 스킬 쿨다운");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(GroundBurst, "GPTags.Cooldown.Skill.GroundBurst", "그라운드 버스트 스킬 쿨다운");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(LightningStrike, "GPTags.Cooldown.Skill.LightningStrike", "라이트닝 스트라이크 스킬 쿨다운");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(ThrownBurst, "GPTags.Cooldown.Skill.ThrownBurst", "투척 버스트 스킬 쿨다운");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(PulseBurst, "GPTags.Cooldown.Skill.PulseBurst", "펄스 버스트 스킬 쿨다운");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(LineShock, "GPTags.Cooldown.Skill.LineShock", "라인 쇼크 스킬 쿨다운");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(MineBurst, "GPTags.Cooldown.Skill.MineBurst", "마인 버스트 스킬 쿨다운");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConeSlash, "GPTags.Cooldown.Skill.ConeSlash", "콘 슬래시 스킬 쿨다운");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(DashSlash, "GPTags.Cooldown.Skill.DashSlash", "대쉬베기 스킬 쿨다운");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(LifeDrainTarget, "GPTags.Cooldown.Skill.LifeDrainTarget", "타겟 흡혈 스킬 쿨다운");
        }
        namespace Data
        {
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Duration, "GPTags.Cooldown.Data.Duration", "SetByCaller 쿨다운 시간");
        }
    }
    
    // [8] AI : BT 연동 논리상태
    namespace AI
    {
        namespace State
        {
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Idle, "GPTags.AI.State.Idle", "대기 중");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Patrol, "GPTags.AI.State.Patrol", "정찰 중");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Chasing, "GPTags.AI.State.Chasing", "추격 중");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat, "GPTags.AI.State.Combat", "교전 중");
        }
    }
}
