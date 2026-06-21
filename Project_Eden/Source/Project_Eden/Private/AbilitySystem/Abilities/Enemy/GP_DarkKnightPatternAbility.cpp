#include "AbilitySystem/Abilities/Enemy/GP_DarkKnightPatternAbility.h"

#include "AI/Debug/EnemyAIDebugUtils.h"
#include "Characters/GP_DarkArmorKnightBossCharacter.h"
#include "Characters/GP_DarkArmorKnightStateComponent.h"
#include "GameplayTags/GP_Tags.h"

UGP_DarkKnightPatternAbility::UGP_DarkKnightPatternAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGP_DarkKnightPatternAbility::SetPatternTag(const FGameplayTag& PatternTag)
{
	FGameplayTagContainer Tags;
	Tags.AddTag(PatternTag);
	SetAssetTags(Tags);
}

bool UGP_DarkKnightPatternAbility::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const AGP_DarkArmorKnightBossCharacter* Boss = ActorInfo ? Cast<AGP_DarkArmorKnightBossCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (!IsValid(Boss))
	{
		return false;
	}
	const UGP_DarkArmorKnightStateComponent* State = Boss->GetDarkKnightStateComponent();
	const FGameplayTag PatternTag = GetPatternTag();
	if (PatternTag == GPTags::Ability::Boss::DarkKnight::Groggy)
	{
		return IsValid(State) && State->IsGuardBroken() && !State->IsGroggy();
	}
	if (PatternTag == GPTags::Ability::Boss::DarkKnight::Counter)
	{
		return IsValid(State) && !State->IsGroggy();
	}
	return IsValid(State)
		&& !State->IsGroggy()
		&& !State->IsGuardBroken()
		&& (!UsesSharedPatternCadence() || Boss->CanStartDarkKnightPattern())
		&& Boss->IsPatternCooldownReady(PatternTag);
}

void UGP_DarkKnightPatternAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AGP_DarkArmorKnightBossCharacter* Boss = Cast<AGP_DarkArmorKnightBossCharacter>(GetAvatarActorFromActorInfo());
	const bool bCadenceReserved = !UsesSharedPatternCadence()
		|| (HasAuthority(&ActivationInfo) && IsValid(Boss) && Boss->TryStartDarkKnightPattern());
	const bool bStarted = bCadenceReserved
		&& HasAuthority(&ActivationInfo)
		&& IsValid(Boss)
		&& ExecuteDarkKnightPattern(Boss, ResolvePatternTarget(TriggerEventData));

	UE_LOG(LogEnemyAI, Log, TEXT("[BossAI] Dark Knight pattern requested. Ability=%s Boss=%s Started=%d"),
		*GetNameSafe(this), *EnemyAIDebugUtils::DescribeActor(Boss), bStarted ? 1 : 0);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, !bStarted);
}

bool UGP_DarkKnightPatternAbility::ExecuteDarkKnightPattern(AGP_DarkArmorKnightBossCharacter* Boss, AActor* TargetActor)
{
	return false;
}

FGameplayTag UGP_DarkKnightPatternAbility::GetPatternTag() const
{
	for (const FGameplayTag& Tag : GetAssetTags())
	{
		return Tag;
	}
	return FGameplayTag();
}

AActor* UGP_DarkKnightPatternAbility::ResolvePatternTarget(const FGameplayEventData* TriggerEventData) const
{
	return TriggerEventData ? const_cast<AActor*>(TriggerEventData->Target.Get()) : nullptr;
}

#define GP_DEFINE_DARK_KNIGHT_ABILITY(ClassName, Tag, ExecuteCall) \
	ClassName::ClassName() { SetPatternTag(Tag); } \
	bool ClassName::ExecuteDarkKnightPattern(AGP_DarkArmorKnightBossCharacter* Boss, AActor* TargetActor) \
	{ \
		return IsValid(Boss) && Boss->ExecuteCall; \
	}

GP_DEFINE_DARK_KNIGHT_ABILITY(UGP_DarkKnightBasicAbility, GPTags::Ability::Boss::DarkKnight::Basic, ExecuteBasicAttack(TargetActor))
GP_DEFINE_DARK_KNIGHT_ABILITY(UGP_DarkKnightHeavyAbility, GPTags::Ability::Boss::DarkKnight::Heavy, ExecuteHeavyAttack(TargetActor))
GP_DEFINE_DARK_KNIGHT_ABILITY(UGP_DarkKnightSweepAbility, GPTags::Ability::Boss::DarkKnight::Sweep, ExecuteSweepAttack(TargetActor))
GP_DEFINE_DARK_KNIGHT_ABILITY(UGP_DarkKnightGuardAbility, GPTags::Ability::Boss::DarkKnight::Guard, ExecuteGuardStance())
GP_DEFINE_DARK_KNIGHT_ABILITY(UGP_DarkKnightChargeAbility, GPTags::Ability::Boss::DarkKnight::Charge, ExecuteChargeAttack(TargetActor))
GP_DEFINE_DARK_KNIGHT_ABILITY(UGP_DarkKnightDarkWaveAbility, GPTags::Ability::Boss::DarkKnight::DarkWave, ExecuteDarkWave(TargetActor))
GP_DEFINE_DARK_KNIGHT_ABILITY(UGP_DarkKnightGroundCrackAbility, GPTags::Ability::Boss::DarkKnight::GroundCrack, ExecuteGroundCrack(TargetActor))

#undef GP_DEFINE_DARK_KNIGHT_ABILITY

UGP_DarkKnightCounterAbility::UGP_DarkKnightCounterAbility()
{
	SetPatternTag(GPTags::Ability::Boss::DarkKnight::Counter);
}

bool UGP_DarkKnightCounterAbility::ExecuteDarkKnightPattern(AGP_DarkArmorKnightBossCharacter* Boss, AActor* TargetActor)
{
	return IsValid(Boss) && Boss->ExecuteCounterAttack(TargetActor);
}

UGP_DarkKnightGroggyAbility::UGP_DarkKnightGroggyAbility()
{
	SetPatternTag(GPTags::Ability::Boss::DarkKnight::Groggy);
}

bool UGP_DarkKnightGroggyAbility::ExecuteDarkKnightPattern(AGP_DarkArmorKnightBossCharacter* Boss, AActor* TargetActor)
{
	return IsValid(Boss) && Boss->ExecuteEnterGroggy();
}
