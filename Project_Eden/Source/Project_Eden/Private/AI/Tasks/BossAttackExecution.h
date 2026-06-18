#pragma once

#include "AI/Tasks/BossAttackPatternSelector.h"
#include "BehaviorTree/BTNode.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

class AActor;
class APawn;
class UAbilitySystemComponent;
class UBlackboardComponent;

namespace BossAttackExecution
{
	bool HasBlackboardKey(const UBlackboardComponent* BlackboardComponent, const FName& KeyName);

	// Older boss BT assets can still point at the generic attack task after merges; this guard keeps boss pattern execution centralized.
	bool ShouldUseBossPatternSelector(const APawn* ControlledPawn, const UBlackboardComponent* BlackboardComponent);

	FGPBossAttackPatternContext BuildPatternContext(
		const APawn* ControlledPawn,
		const UBlackboardComponent* BlackboardComponent,
		const FGameplayTag& DefaultAttackAbilityTag);

	EBTNodeResult::Type ExecuteBestPattern(
		UAbilitySystemComponent* ASC,
		const APawn* ControlledPawn,
		UBlackboardComponent* BlackboardComponent,
		const FGameplayTag& DefaultAttackAbilityTag,
		const AActor* TargetActor);
}
