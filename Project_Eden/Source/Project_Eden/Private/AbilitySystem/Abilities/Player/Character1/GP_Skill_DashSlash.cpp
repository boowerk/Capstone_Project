#include "AbilitySystem/Abilities/Player/Character1/GP_Skill_DashSlash.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GP_SkillData.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/GP_AnimNotify_SendGameplayEvent.h"
#include "Animation/AnimMontage.h"
#include "Animation/PDA_CharacterAnimationSet.h"
#include "Characters/GP_PlayerCharacter.h"
#include "GameFramework/Controller.h"
#include "GameplayEffect.h"
#include "GameplayTags/GP_Tags.h"
#include "UObject/ConstructorHelpers.h"
#include "Utils/GP_BlueprintLibrary.h"

namespace GP_DashSlash_Internal
{
	float FindFirstGameplayEventNotifyTime(const UAnimMontage* Montage, const FGameplayTag& EventTag)
	{
		if (!IsValid(Montage) || !EventTag.IsValid())
		{
			return -1.0f;
		}

		for (const FAnimNotifyEvent& NotifyEvent : Montage->Notifies)
		{
			if (const UGP_AnimNotify_SendGameplayEvent* EventNotify = Cast<UGP_AnimNotify_SendGameplayEvent>(NotifyEvent.Notify))
			{
				if (EventNotify->GameplayEventTag == EventTag)
				{
					return NotifyEvent.GetTime();
				}
			}

			const FString NotifyName = NotifyEvent.NotifyName.ToString();
			if (NotifyName.Equals(EventTag.ToString(), ESearchCase::IgnoreCase)
				|| (EventTag.MatchesTagExact(GPTags::Event::Player::AttackHit) && NotifyName.Equals(TEXT("AttackHit"), ESearchCase::IgnoreCase))
				|| (EventTag.MatchesTagExact(GPTags::Event::Player::ActionEnd) && NotifyName.Equals(TEXT("ActionEnd"), ESearchCase::IgnoreCase)))
			{
				return NotifyEvent.GetTime();
			}
		}

		return -1.0f;
	}
}

UGP_Skill_DashSlash::UGP_Skill_DashSlash()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	ActivationOwnedTags.AddTag(GPTags::State::Movement::Dash);
	CancelAbilitiesWithTag.AddTag(GPTags::Ability::Skill::Primary);

	HitEventTag = GPTags::Event::Enemy::HitReact;

	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageEffectFinder(TEXT("/Game/GAS_Pattern/AbilitySystem/GameplayEffects/Damage/GE_PrimaryDamage"));
	if (DamageEffectFinder.Succeeded())
	{
		DamageEffectClass = DamageEffectFinder.Class;
	}

	static ConstructorHelpers::FClassFinder<UGameplayEffect> CooldownEffectFinder(TEXT("/Game/GAS_Pattern/AbilitySystem/GameplayEffects/Cooldown/GE_Cooldown_Generic"));
	if (CooldownEffectFinder.Succeeded())
	{
		GenericCooldownEffectClass = CooldownEffectFinder.Class;
	}
}

bool UGP_Skill_DashSlash::CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	const UGP_SkillData* SkillData = GetSkillDataFromSpec(Handle, ActorInfo);
	if (!SkillData || SkillData->CooldownPolicy != EGP_CooldownPolicy::Generic || SkillData->CooldownTag.IsValid())
	{
		return Super::CheckCooldown(Handle, ActorInfo, OptionalRelevantTags);
	}

	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC)
	{
		return true;
	}

	if (ASC->HasMatchingGameplayTag(GPTags::Cooldown::Skill::DashSlash))
	{
		if (OptionalRelevantTags)
		{
			OptionalRelevantTags->AddTag(GPTags::Cooldown::Skill::DashSlash);
		}

		return false;
	}

	return true;
}

void UGP_Skill_DashSlash::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	const UGP_SkillData* SkillData = GetSkillDataFromSpec(Handle, ActorInfo);
	if (!SkillData || SkillData->CooldownPolicy != EGP_CooldownPolicy::Generic || SkillData->CooldownTag.IsValid())
	{
		Super::ApplyCooldown(Handle, ActorInfo, ActivationInfo);
		return;
	}

	const float CooldownDuration = SkillData->CooldownDuration > 0.0f ? SkillData->CooldownDuration : DefaultCooldownDuration;
	if (CooldownDuration <= 0.0f || !GenericCooldownEffectClass)
	{
		return;
	}

	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(GenericCooldownEffectClass, GetAbilityLevel(Handle, ActorInfo));
	if (!SpecHandle.IsValid())
	{
		return;
	}

	SpecHandle.Data->DynamicGrantedTags.AddTag(GPTags::Cooldown::Skill::DashSlash);
	SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Cooldown::Data::Duration, CooldownDuration);
	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
}

void UGP_Skill_DashSlash::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                          const FGameplayAbilityActorInfo* ActorInfo,
                                          const FGameplayAbilityActivationInfo ActivationInfo,
                                          const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!ActorInfo)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AGP_PlayerCharacter* PC = Cast<AGP_PlayerCharacter>(ActorInfo->AvatarActor.Get());
	if (!IsValid(PC) || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!ActionRootMotionCancelInputHandle.IsValid())
	{
		ActionRootMotionCancelInputHandle = PC->OnActionRootMotionCancelInput.AddUObject(this, &ThisClass::OnActionRootMotionCancelInput);
	}

	UPDA_CharacterAnimationSet* AnimSet = PC->GetAnimationSet();
	UAnimMontage* MontageToPlay = SkillMontage;
	UAnimMontage* SourceMontageToPlay = nullptr;
	if (IsValid(AnimSet))
	{
		if (!IsValid(MontageToPlay))
		{
			MontageToPlay = AnimSet->SwordMontages.Dash_RM;
		}
		SourceMontageToPlay = AnimSet->SourceSwordMontages.Dash_RM;
	}

	if (!IsValid(MontageToPlay) && !IsValid(SourceMontageToPlay))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FVector DashDirection = PC->GetLastMovementInputVector();
	if (DashDirection.IsNearlyZero())
	{
		if (AController* Controller = PC->GetController())
		{
			DashDirection = Controller->GetControlRotation().Vector();
		}
	}
	if (DashDirection.IsNearlyZero())
	{
		DashDirection = PC->GetActorForwardVector();
	}

	DashDirection.Z = 0.0f;
	if (!DashDirection.IsNearlyZero())
	{
		PC->SetActorRotation(DashDirection.GetSafeNormal().Rotation());
	}

	if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
	{
		ASC->AddLooseGameplayTag(GPTags::State::Status::Fixed);
	}
	bMovementControlUnlocked = false;
	bHasAppliedAttackHit = false;
	ClearExistingTasks();
	PC->BeginActionMotionTracking();

	WaitHitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GPTags::Event::Player::AttackHit);
	if (WaitHitTask)
	{
		WaitHitTask->EventReceived.AddDynamic(this, &ThisClass::OnAttackHitEventReceived);
		WaitHitTask->ReadyForActivation();
	}

	WaitEndTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GPTags::Event::Player::ActionEnd);
	if (WaitEndTask)
	{
		WaitEndTask->EventReceived.AddDynamic(this, &ThisClass::OnActionEndEventReceived);
		WaitEndTask->ReadyForActivation();
	}

	if (IsValid(MontageToPlay))
	{
		MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, MontageToPlay, 1.0f);
		if (!MontageTask)
		{
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
			return;
		}

		MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
		MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageBlendOut);
		MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
		MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageInterrupted);
		MontageTask->ReadyForActivation();
		return;
	}

	const float FallbackDuration = PC->PlayUEFNSourceFallbackMontage(SourceMontageToPlay, 1.0f);
	if (FallbackDuration <= 0.0f)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			FallbackMontageEndTimerHandle,
			this,
			&ThisClass::OnMontageCompleted,
			FallbackDuration,
			false);

		const float AttackHitTime = GP_DashSlash_Internal::FindFirstGameplayEventNotifyTime(SourceMontageToPlay, GPTags::Event::Player::AttackHit);
		if (AttackHitTime >= 0.0f && AttackHitTime < FallbackDuration)
		{
			World->GetTimerManager().SetTimer(
				FallbackAttackHitTimerHandle,
				this,
				&ThisClass::PerformDashSlashHit,
				AttackHitTime,
				false);
		}

		const float ActionEndTime = GP_DashSlash_Internal::FindFirstGameplayEventNotifyTime(SourceMontageToPlay, GPTags::Event::Player::ActionEnd);
		if (ActionEndTime >= 0.0f && ActionEndTime < FallbackDuration)
		{
			World->GetTimerManager().SetTimer(
				FallbackActionEndTimerHandle,
				this,
				&ThisClass::OnFallbackActionEnd,
				ActionEndTime,
				false);
		}
	}
}

void UGP_Skill_DashSlash::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                     const FGameplayAbilityActorInfo* ActorInfo,
                                     const FGameplayAbilityActivationInfo ActivationInfo,
                                     bool bReplicateEndAbility,
                                     bool bWasCancelled)
{
	UAnimMontage* ActiveMontage = nullptr;
	float ActiveMontagePosition = -1.0f;
	if (AGP_PlayerCharacter* PC = Cast<AGP_PlayerCharacter>(GetAvatarActorFromActorInfo()))
	{
		if (UAnimInstance* AnimInstance = PC->GetMesh() ? PC->GetMesh()->GetAnimInstance() : nullptr)
		{
			ActiveMontage = AnimInstance->GetCurrentActiveMontage();
			ActiveMontagePosition = IsValid(ActiveMontage) ? AnimInstance->Montage_GetPosition(ActiveMontage) : -1.0f;
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("[ActionEndTrace][DashSlash] EndAbility Cancelled=%d ActiveMontage=%s Pos=%.3f Hit=%d MovementUnlocked=%d"),
		bWasCancelled ? 1 : 0,
		*GetNameSafe(ActiveMontage),
		ActiveMontagePosition,
		bHasAppliedAttackHit ? 1 : 0,
		bMovementControlUnlocked ? 1 : 0);

	UnlockMovementControl();
	ClearExistingTasks();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FallbackMontageEndTimerHandle);
		World->GetTimerManager().ClearTimer(FallbackAttackHitTimerHandle);
		World->GetTimerManager().ClearTimer(FallbackActionEndTimerHandle);
	}

	if (AGP_PlayerCharacter* PC = Cast<AGP_PlayerCharacter>(GetAvatarActorFromActorInfo()))
	{
		PC->SetActionRootMotionInputCancelEnabled(false);
		if (ActionRootMotionCancelInputHandle.IsValid())
		{
			PC->OnActionRootMotionCancelInput.Remove(ActionRootMotionCancelInputHandle);
			ActionRootMotionCancelInputHandle.Reset();
		}

		if (!bWasCancelled)
		{
			PC->ApplyCurrentActionInertia();
		}
		else
		{
			PC->StopActionMotionTracking();
		}
		PC->StopUEFNSourceFallbackMontage(0.2f);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGP_Skill_DashSlash::ClearExistingTasks()
{
	if (MontageTask)
	{
		MontageTask->EndTask();
		MontageTask = nullptr;
	}
	if (WaitHitTask)
	{
		WaitHitTask->EndTask();
		WaitHitTask = nullptr;
	}
	if (WaitEndTask)
	{
		WaitEndTask->EndTask();
		WaitEndTask = nullptr;
	}
}

void UGP_Skill_DashSlash::PerformDashSlashHit()
{
	if (bHasAppliedAttackHit)
	{
		return;
	}

	bHasAppliedAttackHit = true;

	if (!HasAuthority(&CurrentActivationInfo))
	{
		return;
	}

	AGP_PlayerCharacter* Avatar = Cast<AGP_PlayerCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(Avatar))
	{
		return;
	}

	UGP_SkillData* SkillData = GetSkillDataFromSpec(CurrentSpecHandle, CurrentActorInfo);
	const FGameplayTag TechElementTag = GetCurrentTechElementTag(CurrentActorInfo);

	const FRotator HitRotation(0.0f, Avatar->GetActorRotation().Yaw, 0.0f);
	const FVector Forward = HitRotation.Vector();
	const FVector Origin = Avatar->GetActorLocation();
	const FVector HitCenter = Origin + Forward * (HitRange * 0.5f) + FVector::UpVector * HitHeightOffset;
	const FVector HitExtent(HitRange * 0.5f, HitWidth * 0.5f, HitHeight * 0.5f);

	const FVector VisualLocation = Origin + Forward * VisualForwardOffset + FVector::UpVector * VisualHeightOffset;
	SpawnVisualActor(Avatar, GetSkillVisualActorClass(SkillData, SlashVisualActorClass, TechElementTag), VisualLocation, HitRotation);

	const TArray<AActor*> HitActors = UGP_BlueprintLibrary::BoxOverlapActorsAtLocation(
		Avatar,
		HitCenter,
		HitExtent,
		HitRotation,
		Avatar,
		bDrawDebugs);

	UGP_BlueprintLibrary::ApplyGameplayEffectAndEventToActors(Avatar, HitActors, DamageEffectClass, HitEventTag, GetAbilityLevel(), SkillData);
}

void UGP_Skill_DashSlash::UnlockMovementControl()
{
	if (bMovementControlUnlocked)
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr)
	{
		ASC->RemoveLooseGameplayTag(GPTags::State::Status::Fixed);
	}

	bMovementControlUnlocked = true;
}

void UGP_Skill_DashSlash::OnAttackHitEventReceived(FGameplayEventData Payload)
{
	PerformDashSlashHit();
}

void UGP_Skill_DashSlash::OnActionEndEventReceived(FGameplayEventData Payload)
{
	UE_LOG(LogTemp, Warning, TEXT("[ActionEndTrace][DashSlash] ActionEndEvent UnlockOnly"));
	UnlockMovementControl();
	if (AGP_PlayerCharacter* PC = Cast<AGP_PlayerCharacter>(GetAvatarActorFromActorInfo()))
	{
		PC->SetActionLowerBodyMotionMatchBlendEnabled(true);
		PC->SetActionRootMotionInputCancelEnabled(true);
	}

	if (!bHasAppliedAttackHit)
	{
		PerformDashSlashHit();
	}
}

void UGP_Skill_DashSlash::OnFallbackActionEnd()
{
	UE_LOG(LogTemp, Warning, TEXT("[ActionEndTrace][DashSlash] FallbackActionEndTimer UnlockOnly"));
	UnlockMovementControl();
	if (AGP_PlayerCharacter* PC = Cast<AGP_PlayerCharacter>(GetAvatarActorFromActorInfo()))
	{
		PC->SetActionLowerBodyMotionMatchBlendEnabled(true);
		PC->SetActionRootMotionInputCancelEnabled(true);
	}

	if (!bHasAppliedAttackHit)
	{
		PerformDashSlashHit();
	}
}

void UGP_Skill_DashSlash::OnActionRootMotionCancelInput()
{
	UE_LOG(LogTemp, Warning, TEXT("[ActionEndTrace][DashSlash] InputCancelDelegate EndAbility"));
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGP_Skill_DashSlash::OnMontageCompleted()
{
	UE_LOG(LogTemp, Warning, TEXT("[ActionEndTrace][DashSlash] MontageCompleted EndAbility"));
	if (!bHasAppliedAttackHit)
	{
		PerformDashSlashHit();
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGP_Skill_DashSlash::OnMontageBlendOut()
{
	UE_LOG(LogTemp, Warning, TEXT("[ActionEndTrace][DashSlash] MontageBlendOut LogOnly"));
}

void UGP_Skill_DashSlash::OnMontageInterrupted()
{
	UE_LOG(LogTemp, Warning, TEXT("[ActionEndTrace][DashSlash] MontageInterrupted EndAbility"));
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
