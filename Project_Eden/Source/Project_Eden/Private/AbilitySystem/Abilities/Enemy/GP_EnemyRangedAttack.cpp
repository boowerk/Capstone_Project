#include "AbilitySystem/Abilities/Enemy/GP_EnemyRangedAttack.h"

#include "GameplayTags/GP_Tags.h"

UGP_EnemyRangedAttack::UGP_EnemyRangedAttack()
{
	FGameplayTagContainer AbilityAssetTags;
	AbilityAssetTags.AddTag(GPTags::Ability::Enemy::Attack_Ranged);
	SetAssetTags(AbilityAssetTags);

	// This first ranged prototype reuses the shared enemy hit logic at a far forward point; projectile visuals can replace it later.
	AttackRadius = 260.0f;
	ForwardOffset = 900.0f;
}
