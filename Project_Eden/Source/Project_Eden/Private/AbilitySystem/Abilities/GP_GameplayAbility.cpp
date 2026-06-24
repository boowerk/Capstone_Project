#include "AbilitySystem/Abilities/GP_GameplayAbility.h"

#include "HAL/IConsoleManager.h"

namespace
{
	// Master switch for skill debug drawing (overlap shapes, on-screen "Activated" messages).
	// Per-ability bDrawDebugs must also be enabled; this only lets us mute everything for video capture.
	static TAutoConsoleVariable<int32> CVarDrawSkillDebug(
		TEXT("g.DrawSkillDebug"),
		1,
		TEXT("Global gate for skill debug drawing.\n")
		TEXT("  0: suppress all skill debug draws regardless of per-ability bDrawDebugs\n")
		TEXT("  1: allow (default); per-ability bDrawDebugs still decides each skill"),
		ECVF_Default);
}

bool UGP_GameplayAbility::IsSkillDebugDrawEnabled()
{
	return CVarDrawSkillDebug.GetValueOnGameThread() != 0;
}

bool UGP_GameplayAbility::ShouldDrawDebug() const
{
	return bDrawDebugs && IsSkillDebugDrawEnabled();
}

void UGP_GameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (ShouldDrawDebug() && IsValid(GEngine))
	{
		const FString DebugMessage = FString::Printf(TEXT("%s Activated"), *GetName());
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.0f, FColor::Cyan, DebugMessage);
	}
}
