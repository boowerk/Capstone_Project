#include "AI/Tasks/BTT_BossSweepAttack.h"

#include "GameplayTags/GP_Tags.h"

UBTT_BossSweepAttack::UBTT_BossSweepAttack()
{
	NodeName = TEXT("Boss Sweep Attack");
	// 전방 부채꼴 판정을 쓰는 보스 휩쓸기 어빌리티 태그를 실행합니다.
	AttackAbilityTag = GPTags::Ability::Enemy::Attack_BossSweep;
}
