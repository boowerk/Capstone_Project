#include "AbilitySystem/Abilities/GP_GameplayAbility.h"

#include "Game/GP_GameState.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "Player/GP_PlayerState.h"

namespace
{
	// Master opt-in for skill debug drawing. Per-ability debug flags must also be enabled.
	static TAutoConsoleVariable<int32> CVarDrawSkillDebug(
		TEXT("g.DrawSkillDebug"),
		0,
		TEXT("Global gate for skill debug drawing.\n")
		TEXT("  0: suppress all skill debug draws regardless of per-ability bDrawDebugs\n")
		TEXT("  1: allow in non-Shipping builds; per-ability debug flags still decide each skill"),
		ECVF_Cheat);
}

bool UGP_GameplayAbility::IsSkillDebugDrawEnabled()
{
#if UE_BUILD_SHIPPING
	return false;
#else
	return CVarDrawSkillDebug.GetValueOnGameThread() != 0;
#endif
}

bool UGP_GameplayAbility::ShouldDrawDebug() const
{
	return bDrawDebugs && IsSkillDebugDrawEnabled();
}

bool UGP_GameplayAbility::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(
		Handle,
		ActorInfo,
		SourceTags,
		TargetTags,
		OptionalRelevantTags))
	{
		return false;
	}

	const AActor* OwnerActor = ActorInfo ? ActorInfo->OwnerActor.Get() : nullptr;
	const AActor* AvatarActor = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	const AGP_PlayerState* GPPlayerState = Cast<AGP_PlayerState>(OwnerActor);
	if (!IsValid(GPPlayerState))
	{
		if (const APawn* AvatarPawn = Cast<APawn>(AvatarActor))
		{
			GPPlayerState = AvatarPawn->GetPlayerState<AGP_PlayerState>();
		}
	}
	if (IsValid(GPPlayerState) && GPPlayerState->IsEliminated())
	{
		// The authority repeats this check when it validates a predicted client
		// activation, closing the replication-delay window after elimination.
		return false;
	}

	const AActor* WorldContextActor = IsValid(AvatarActor) ? AvatarActor : OwnerActor;
	const AGP_GameState* GPGameState = IsValid(WorldContextActor) && WorldContextActor->GetWorld()
		? WorldContextActor->GetWorld()->GetGameState<AGP_GameState>()
		: nullptr;
	return !IsValid(GPGameState)
		|| (GPGameState->GetMatchPhase() != EGPMatchPhase::Victory
			&& GPGameState->GetMatchPhase() != EGPMatchPhase::Defeat);
}

void UGP_GameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}
