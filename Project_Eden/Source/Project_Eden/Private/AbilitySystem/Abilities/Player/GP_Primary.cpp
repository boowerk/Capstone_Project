#include "AbilitySystem/Abilities/Player/GP_Primary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Characters/GP_PlayerCharacter.h"
#include "GameplayTags/GP_Tags.h"
#include "Animation/PDA_CharacterAnimationSet.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Player/GP_PlayerController.h"
#include "Utils/GP_BlueprintLibrary.h"

UGP_Primary::UGP_Primary()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 어빌리티 활성화 동안 이동 입력을 막기 위해 Fixed 태그 부여
	ActivationOwnedTags.AddTag(GPTags::State::Status::Fixed);

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
}
void UGP_Primary::StartComboSequence()
{
	bHasQueuedNextAttack = false;
	bIsComboWindowOpen = false;

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

	if (!IsValid(MontageToPlay) && !IsValid(SourceMontageToPlay))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}
	
	if (AGP_PlayerController* PCtrl = Cast<AGP_PlayerController>(PC->GetController()))
	{
		FVector2D MoveInput = PCtrl->GetCurrentMoveInput();
		if (!MoveInput.IsNearlyZero())
		{
			const FRotator YawRotation(0.f, PCtrl->GetControlRotation().Yaw, 0.f);
			const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
			const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

			// 2D 입력 벡터를 3D 월드 방향 벡터로 변환
			FVector DesiredDirection = (ForwardDirection * MoveInput.Y) + (RightDirection * MoveInput.X);
			DesiredDirection.Z = 0.0f; 
			DesiredDirection.Normalize();

			// 캐릭터의 방향을 즉시 변경
			PC->SetActorRotation(DesiredDirection.Rotation());
		}
	}

	ClearExistingTasks();

	WaitHitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GPTags::Event::Player::AttackHit);
	WaitHitTask->EventReceived.AddDynamic(this, &ThisClass::OnAttackHitEventReceived);
	WaitHitTask->ReadyForActivation();

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
	else if (PC->PlayUEFNSourceFallbackMontage(SourceMontageToPlay, 1.0f) <= 0.0f)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

void UGP_Primary::ClearExistingTasks()
{
	if (MontageTask) { MontageTask->EndTask(); MontageTask = nullptr; }
	if (WaitHitTask) { WaitHitTask->EndTask(); WaitHitTask = nullptr; }
	if (WaitComboTask) { WaitComboTask->EndTask(); WaitComboTask = nullptr; }
	if (WaitEndTask) { WaitEndTask->EndTask(); WaitEndTask = nullptr; }
}

int32 UGP_Primary::GetNextComboIndex(int32 MaxComboCount)
{
	if (bUseRandomCombo) return FMath::RandRange(0, MaxComboCount - 1);
	return (CurrentComboIndex + 1) % MaxComboCount;
}


void UGP_Primary::OnComboEnableEventReceived(FGameplayEventData Payload)
{
	bIsComboWindowOpen = true;
}

void UGP_Primary::OnActionEndEventReceived(FGameplayEventData Payload)
{
	AGP_PlayerCharacter* PC = Cast<AGP_PlayerCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(PC) || !IsValid(PC->GetAnimationSet()))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	const UPDA_CharacterAnimationSet* AnimSet = PC->GetAnimationSet();
	int32 MaxCombo = FMath::Max(AnimSet->LightAttackMontages.Num(), AnimSet->SourceLightAttackMontages.Num());
	if (MaxCombo <= 0)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	if (bHasQueuedNextAttack)
	{
		CurrentComboIndex = GetNextComboIndex(MaxCombo);

		if (!bUseRandomCombo && CurrentComboIndex == 0)
		{
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		}
		else
		{
			StartComboSequence();
		}
	}
	else
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UGP_Primary::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGP_Primary::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGP_Primary::OnAttackHitEventReceived(FGameplayEventData Payload)
{
	// 메커니즘: 부모 클래스의 공통 공격 판정 실행
	PerformAreaAttack();
}

void UGP_Primary::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	ClearExistingTasks();

	if (AGP_PlayerCharacter* PC = Cast<AGP_PlayerCharacter>(GetAvatarActorFromActorInfo()))
	{
		PC->StopUEFNSourceFallbackMontage(0.2f);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
