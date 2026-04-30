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
	UE_LOG(LogTemp, Warning, TEXT("UGP_Primary : InputPressed Called - Next Attack Queued!"));
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
	if (!IsValid(AnimSet) || AnimSet->LightAttackMontages.IsEmpty())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	UAnimMontage* MontageToPlay = nullptr;
	if (AnimSet->LightAttackMontages.IsValidIndex(CurrentComboIndex))
	{
		MontageToPlay = AnimSet->LightAttackMontages[CurrentComboIndex];
	}

	if (!IsValid(MontageToPlay))
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

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, MontageToPlay, 1.0f);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	MontageTask->ReadyForActivation();
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
	UE_LOG(LogTemp, Warning, TEXT("UGP_Primary : OnComboEnableEventReceived Called - Combo Window Open"));
	bIsComboWindowOpen = true;
}

void UGP_Primary::OnActionEndEventReceived(FGameplayEventData Payload)
{
	UE_LOG(LogTemp, Warning, TEXT("UGP_Primary : OnActionEndEventReceived Called - bHasQueuedNextAttack : %s"), bHasQueuedNextAttack ? TEXT("True") : TEXT("False"));
	AGP_PlayerCharacter* PC = Cast<AGP_PlayerCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(PC) || !IsValid(PC->GetAnimationSet()))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	int32 MaxCombo = PC->GetAnimationSet()->LightAttackMontages.Num();

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
	TArray<AActor*> HitActors = UGP_BlueprintLibrary::SphereMeleeHitBoxOverlap(
	   GetAvatarActorFromActorInfo(), HitBoxRadius, HitBoxForwardOffset, HitBoxElevationOffset, bDrawDebugs);

	UGP_BlueprintLibrary::SendGameplayEventToActors(GetAvatarActorFromActorInfo(), HitActors,
										GPTags::Event::Enemy::HitReact);

	if (HasAuthority(&CurrentActivationInfo))
	{
		UGP_BlueprintLibrary::ApplyGameplayEffectToActors(GetAvatarActorFromActorInfo(), HitActors, DamageEffectClass,
											  GetAbilityLevel());
	}
}
