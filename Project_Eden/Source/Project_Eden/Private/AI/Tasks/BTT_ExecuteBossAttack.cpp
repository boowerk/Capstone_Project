#include "AI/Tasks/BTT_ExecuteBossAttack.h"

UBTT_ExecuteBossAttack::UBTT_ExecuteBossAttack()
{
	// Boss BTs use the shared activation path, but this node labels the asset as boss-specific.
	NodeName = TEXT("Execute Boss Attack");
	BossSweepAttackChance = 0.5f;
}
