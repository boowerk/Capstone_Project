#include "AI/Tasks/BTT_BossHeavyAttack.h"

#include "GameplayTags/GP_Tags.h"

UBTT_BossHeavyAttack::UBTT_BossHeavyAttack()
{
	NodeName = TEXT("Boss Heavy Attack");
	// 기존 공격 실행 태스크를 재사용하되 보스 강공격 태그만 고정한다.
	AttackAbilityTag = GPTags::Ability::Enemy::Attack_BossHeavy;
}
