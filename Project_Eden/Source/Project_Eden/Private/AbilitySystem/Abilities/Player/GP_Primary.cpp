#include "AbilitySystem/Abilities/Player/GP_Primary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Characters/GP_PlayerCharacter.h"
#include "GameplayTags/GP_Tags.h"
#include "Animation/PDA_CharacterAnimationSet.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Utils/GP_BlueprintLibrary.h"

UGP_Primary::UGP_Primary()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	FGameplayTagContainer AbilityAssetTags;
	AbilityAssetTags.AddTag(GPTags::Ability::Skill::Primary);
	SetAssetTags(AbilityAssetTags);

	// 부모 클래스 수치 기본값 설정
	AttackRadius = 100.0f;
	ForwardOffset = 200.0f;
}

void UGP_Primary::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CurrentComboIndex = 0;
	StartComboSequence();
}

void UGP_Primary::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);

	// 선입력 허용: 애니메이션 도중 언제 클릭하든 다음 공격을 예약합니다.
	bHasQueuedNextAttack = true;
	TryStartQueuedAttackFromActionEnd();
}
void UGP_Primary::StartComboSequence()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SourceFallbackCompletionTimerHandle);
	}
	bActionEndWindowOpen = false;

	bHasQueuedNextAttack = false;
	bIsComboWindowOpen = false;
	bHasAppliedCurrentAttackHit = false;
	bCurrentSwingUsesActionMotionTracking = ShouldUseActionMotionTrackingForComboIndex(CurrentComboIndex);

	AGP_PlayerCharacter* PC = Cast<AGP_PlayerCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(PC))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	UPDA_CharacterAnimationSet* AnimSet = PC->GetAnimationSet();
	if (!IsValid(AnimSet) || (AnimSet->LightAttackMontages.IsEmpty() && AnimSet->SourceLightAttackMontages.IsEmpty()))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	UAnimMontage* MontageToPlay = nullptr;
	UAnimMontage* SourceMontageToPlay = nullptr;
	if (AnimSet->LightAttackMontages.IsValidIndex(CurrentComboIndex))
	{
		MontageToPlay = AnimSet->LightAttackMontages[CurrentComboIndex];
	}
	if (AnimSet->SourceLightAttackMontages.IsValidIndex(CurrentComboIndex))
	{
		SourceMontageToPlay = AnimSet->SourceLightAttackMontages[CurrentComboIndex];
	}
	CurrentTargetAttackMontage = MontageToPlay;
	CurrentSourceAttackMontage = SourceMontageToPlay;
	UE_LOG(LogTemp, Warning, TEXT("[PrimaryCombo] Start Index=%d Target=%s Source=%s ActionRM=%d"),
		CurrentComboIndex,
		*GetNameSafe(MontageToPlay),
		*GetNameSafe(SourceMontageToPlay),
		bCurrentSwingUsesActionMotionTracking ? 1 : 0);

	if (!IsValid(MontageToPlay) && !IsValid(SourceMontageToPlay))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}
	

	ClearExistingTasks();
	PC->AimPrimaryAttackAtBestTarget(AutoTargetSearchRadius, ForwardOffset, AutoTargetFacingDuration);
	if (bCurrentSwingUsesActionMotionTracking)
	{
		PC->StopPrimaryAttackMovementAssist();
		PC->BeginActionMotionTracking();
	}
	else
	{
		PC->StopActionMotionTracking();
		PC->SetActionLowerBodyMotionMatchBlendEnabled(true);
		PC->BeginPrimaryAttackMovementAssist(MobileAttackMovementAssistSpeedRatio);
	}

	WaitHitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GPTags::Event::Player::AttackHit);
	WaitHitTask->EventReceived.AddDynamic(this, &ThisClass::OnAttackHitEventReceived);
	WaitHitTask->ReadyForActivation();

	// Compatibility for migrated montages that still use the primary ability tag as the hit-frame event.
	WaitLegacyPrimaryHitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GPTags::Ability::Skill::Primary);
	WaitLegacyPrimaryHitTask->EventReceived.AddDynamic(this, &ThisClass::OnAttackHitEventReceived);
	WaitLegacyPrimaryHitTask->ReadyForActivation();

	WaitComboTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GPTags::Event::Player::ComboEnable);
	WaitComboTask->EventReceived.AddDynamic(this, &ThisClass::OnComboEnableEventReceived);
	WaitComboTask->ReadyForActivation();

	WaitEndTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GPTags::Event::Player::ActionEnd);
	WaitEndTask->EventReceived.AddDynamic(this, &ThisClass::OnActionEndEventReceived);
	WaitEndTask->ReadyForActivation();

	if (IsValid(MontageToPlay))
	{
		MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, MontageToPlay, 1.0f);
		MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
		MontageTask->ReadyForActivation();
	}
	else
	{
		const float SourceFallbackDuration = PC->PlayUEFNSourceFallbackMontage(SourceMontageToPlay, 1.0f);
		if (SourceFallbackDuration <= 0.0f)
		{
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
			return;
		}
		ScheduleSourceFallbackMontageCompletion(SourceFallbackDuration);
	}
}

void UGP_Primary::ClearExistingTasks()
{
	if (MontageTask) { MontageTask->EndTask(); MontageTask = nullptr; }
	ClearEventTasks();
}

void UGP_Primary::ClearEventTasks()
{
	if (WaitHitTask) { WaitHitTask->EndTask(); WaitHitTask = nullptr; }
	if (WaitLegacyPrimaryHitTask) { WaitLegacyPrimaryHitTask->EndTask(); WaitLegacyPrimaryHitTask = nullptr; }
	if (WaitComboTask) { WaitComboTask->EndTask(); WaitComboTask = nullptr; }
	if (WaitEndTask) { WaitEndTask->EndTask(); WaitEndTask = nullptr; }
}

void UGP_Primary::ScheduleSourceFallbackMontageCompletion(float Duration)
{
	if (Duration <= KINDA_SMALL_NUMBER)
	{
		OnSourceFallbackMontageCompleted();
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[PrimaryCombo] SourceFallbackCompletionTimer Duration=%.3f"), Duration);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			SourceFallbackCompletionTimerHandle,
			this,
			&ThisClass::OnSourceFallbackMontageCompleted,
			Duration,
			false);
	}
}

void UGP_Primary::OnSourceFallbackMontageCompleted()
{
	OnMontageCompleted();
}

bool UGP_Primary::TryStartQueuedAttackFromActionEnd()
{
	if (!bActionEndWindowOpen || !bHasQueuedNextAttack)
	{
		return false;
	}

	AGP_PlayerCharacter* PC = Cast<AGP_PlayerCharacter>(GetAvatarActorFromActorInfo());
	const UPDA_CharacterAnimationSet* AnimSet = IsValid(PC) ? PC->GetAnimationSet() : nullptr;
	const int32 MaxCombo = AnimSet ? FMath::Max(AnimSet->LightAttackMontages.Num(), AnimSet->SourceLightAttackMontages.Num()) : 0;
	if (MaxCombo <= 0 || CurrentComboIndex >= MaxCombo - 1)
	{
		return false;
	}

	CurrentComboIndex = GetNextComboIndex(MaxCombo);
	StartComboSequence();
	return true;
}

int32 UGP_Primary::GetNextComboIndex(int32 MaxComboCount)
{
	if (bUseRandomCombo) return FMath::RandRange(0, MaxComboCount - 1);
	return (CurrentComboIndex + 1) % MaxComboCount;
}

bool UGP_Primary::ShouldUseActionMotionTrackingForComboIndex(int32 ComboIndex) const
{
	return ActionMotionComboIndices.Contains(ComboIndex);
}


void UGP_Primary::OnComboEnableEventReceived(FGameplayEventData Payload)
{
	bIsComboWindowOpen = true;
}

void UGP_Primary::OnActionEndEventReceived(FGameplayEventData Payload)
{
	bActionEndWindowOpen = true;
	if (TryStartQueuedAttackFromActionEnd())
	{
		return;
	}
}

void UGP_Primary::OnMontageCompleted()
{
	AGP_PlayerCharacter* PC = Cast<AGP_PlayerCharacter>(GetAvatarActorFromActorInfo());
	const UPDA_CharacterAnimationSet* AnimSet = IsValid(PC) ? PC->GetAnimationSet() : nullptr;
	const int32 MaxCombo = AnimSet ? FMath::Max(AnimSet->LightAttackMontages.Num(), AnimSet->SourceLightAttackMontages.Num()) : 0;
	if (MaxCombo > 0 && CurrentComboIndex < MaxCombo - 1)
	{
		if (bHasQueuedNextAttack)
		{
			UE_LOG(LogTemp, Warning, TEXT("[PrimaryCombo] MontageCompletedStartQueued Index=%d Max=%d"), CurrentComboIndex, MaxCombo);
			CurrentComboIndex = GetNextComboIndex(MaxCombo);
			StartComboSequence();
			return;
		}
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGP_Primary::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGP_Primary::OnAttackHitEventReceived(FGameplayEventData Payload)
{
	if (bHasAppliedCurrentAttackHit)
	{
		return;
	}

	bHasAppliedCurrentAttackHit = true;

	// 메커니즘: 부모 클래스의 공통 공격 판정 실행
	PerformAreaAttack();
}

void UGP_Primary::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SourceFallbackCompletionTimerHandle);
	}
	bActionEndWindowOpen = false;
	ClearExistingTasks();

	if (AGP_PlayerCharacter* PC = Cast<AGP_PlayerCharacter>(GetAvatarActorFromActorInfo()))
	{
		if (!bWasCancelled && bCurrentSwingUsesActionMotionTracking)
		{
			PC->ApplyCurrentActionInertia();
		}
		else
		{
			PC->StopActionMotionTracking();
		}
		PC->StopPrimaryAttackMovementAssist();
		PC->StopUEFNSourceFallbackMontage(0.2f);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
