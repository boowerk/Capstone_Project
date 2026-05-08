#include "AI/Tasks/BTT_BossSummonAdds.h"

#include "GameplayTags/GP_Tags.h"

UBTT_BossSummonAdds::UBTT_BossSummonAdds()
{
	NodeName = TEXT("Boss Summon Adds");
	// 3페이즈에서 잡몹 소환 패턴 어빌리티 태그를 실행한다.
	AttackAbilityTag = GPTags::Ability::Enemy::Utility_BossSummon;
}
