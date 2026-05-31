#include "AbilitySystem/Abilities/Player/GP_Dash.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/GP_AnimNotify_SendGameplayEvent.h"
#include "Animation/AnimInstance.h"
#include "Characters/GP_PlayerCharacter.h"
#include "GameplayTags/GP_Tags.h"
#include "Animation/PDA_CharacterAnimationSet.h"
#include "TimerManager.h"

namespace
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
		if (NotifyName.Equals(TEXT("ActionEnd"), ESearchCase::IgnoreCase) || NotifyName.Equals(EventTag.ToString(), ESearchCase::IgnoreCase))
		{
			return NotifyEvent.GetTime();
		}
	}

	return -1.0f;
}
}

UGP_Dash::UGP_Dash()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	// 어빌리티 고유 식별 태그
	FGameplayTagContainer AbilityAssetTags;
	AbilityAssetTags.AddTag(GPTags::Ability::Movement::Dash);
	SetAssetTags(AbilityAssetTags);

	ActivationOwnedTags.AddTag(GPTags::State::Movement::Dash);       // 대시 상태 
	ActivationOwnedTags.AddTag(GPTags::State::Status::Fixed);		// 이동 및 회전 입력 차단
	// ActivationOwnedTags.AddTag(GPTags::State::Status::Unstoppable);	// 저지불가 상태
	// ActivationOwnedTags.AddTag(GPTags::State::Status::Invincible);	// 무적 상태
	BlockAbilitiesWithTag.AddTag(GPTags::Ability::Skill::SkillRoot);
	CancelAbilitiesWithTag.AddTag(GPTags::Ability::Skill::Primary);
}

void UGP_Dash::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AGP_PlayerCharacter* PC = Cast<AGP_PlayerCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(PC))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!ActionRootMotionCancelInputHandle.IsValid())
	{
		ActionRootMotionCancelInputHandle = PC->OnActionRootMotionCancelInput.AddUObject(this, &ThisClass::OnActionRootMotionCancelInput);
	}

	// 애니메이션 세트 - 구르기(Dash) 몽타주
	UAnimMontage* DashMontage = nullptr;
	UAnimMontage* SourceDashMontage = nullptr;
	if (UPDA_CharacterAnimationSet* AnimSet = PC->GetAnimationSet())
	{
		DashMontage = AnimSet->RollMontages.Roll_RM;
		SourceDashMontage = AnimSet->SourceRollMontages.Roll_RM;
	}

	if (!IsValid(DashMontage) && !IsValid(SourceDashMontage))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 캐릭터 회전 로직: 입력 방향이 없으면 바라보는 방향, 있으면 입력 방향으로 즉시 회전
	FVector DashDirection = PC->GetLastMovementInputVector();
	if (DashDirection.IsNearlyZero())
	{
		DashDirection = PC->GetActorForwardVector();
	}
    
	DashDirection.Z = 0.0f;
    
	if (!DashDirection.IsNearlyZero())
	{
		DashDirection.Normalize();
		PC->SetActorRotation(DashDirection.Rotation());
	}

	PC->BeginActionMotionTracking();
	ActiveDashMontage = DashMontage;

	if (IsValid(DashMontage))
	{
		// 몽타주 실행 태스크 생성
		UAbilityTask_PlayMontageAndWait* PlayTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, DashMontage, 1.0f);

		if (PlayTask)
		{
			PlayTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
			PlayTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
			PlayTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageInterrupted);
			PlayTask->ReadyForActivation();
		}
		else
		{
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
			return;
		}
	}
	else
	{
		ActiveDashMontage = nullptr;
		const float FallbackDuration = PC->PlayUEFNSourceFallbackMontage(SourceDashMontage, 1.0f);
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

			const float ActionEndTime = FindFirstGameplayEventNotifyTime(SourceDashMontage, GPTags::Event::Player::ActionEnd);
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

	UAbilityTask_WaitGameplayEvent* WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, GPTags::Event::Player::ActionEnd);

	if (WaitEventTask)
	{
		WaitEventTask->EventReceived.AddDynamic(this, &ThisClass::OnDashActionEnd);
		WaitEventTask->ReadyForActivation();
	}
}

void UGP_Dash::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	UE_LOG(LogTemp, Warning, TEXT("[ActionEndTrace][Dash] EndAbility Cancelled=%d ActiveTarget=%s"),
		bWasCancelled ? 1 : 0,
		*GetNameSafe(ActiveDashMontage));

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FallbackMontageEndTimerHandle);
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

		if (IsValid(ActiveDashMontage))
		{
			if (UAnimInstance* AnimInstance = PC->GetMesh() ? PC->GetMesh()->GetAnimInstance() : nullptr)
			{
				AnimInstance->Montage_Stop(0.2f, ActiveDashMontage);
			}
		}
		ActiveDashMontage = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGP_Dash::OnDashActionEnd(FGameplayEventData Payload)
{
	UE_LOG(LogTemp, Warning, TEXT("[ActionEndTrace][Dash] ActionEndEvent EnableInputCancel"));
	if (AGP_PlayerCharacter* PC = Cast<AGP_PlayerCharacter>(GetAvatarActorFromActorInfo()))
	{
		PC->SetActionRootMotionInputCancelEnabled(true);
	}
}

void UGP_Dash::OnFallbackActionEnd()
{
	UE_LOG(LogTemp, Warning, TEXT("[ActionEndTrace][Dash] FallbackActionEndTimer EnableInputCancel"));
	if (AGP_PlayerCharacter* PC = Cast<AGP_PlayerCharacter>(GetAvatarActorFromActorInfo()))
	{
		PC->SetActionRootMotionInputCancelEnabled(true);
	}
}

void UGP_Dash::OnActionRootMotionCancelInput()
{
	UE_LOG(LogTemp, Warning, TEXT("[ActionEndTrace][Dash] InputCancelDelegate EndAbility"));
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGP_Dash::OnMontageCompleted()
{
	UE_LOG(LogTemp, Warning, TEXT("[ActionEndTrace][Dash] MontageCompleted EndAbility"));
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGP_Dash::OnMontageInterrupted()
{
	UE_LOG(LogTemp, Warning, TEXT("[ActionEndTrace][Dash] MontageInterrupted EndAbility"));
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
