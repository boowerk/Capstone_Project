#include "AI/Tasks/BTT_BossPhaseShift.h"

#include "GameplayTags/GP_Tags.h"

UBTT_BossPhaseShift::UBTT_BossPhaseShift()
{
	NodeName = TEXT("Boss Phase Shift");
	// 보스 페이즈 전환 연출/버프 어빌리티 태그를 실행한다.
	AttackAbilityTag = GPTags::Ability::Enemy::Utility_BossPhaseShift;
}
