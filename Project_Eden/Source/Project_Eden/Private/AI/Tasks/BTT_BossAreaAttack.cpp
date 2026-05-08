#include "AI/Tasks/BTT_BossAreaAttack.h"

#include "GameplayTags/GP_Tags.h"

UBTT_BossAreaAttack::UBTT_BossAreaAttack()
{
	NodeName = TEXT("Boss Area Attack");
	// 2페이즈 이후 사용하는 보스 광역 공격 어빌리티 태그를 실행한다.
	AttackAbilityTag = GPTags::Ability::Enemy::Attack_BossArea;
}
