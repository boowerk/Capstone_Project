#include "AbilitySystem/Abilities/Player/GP_Primary.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/GP_AttributeSet.h"
#include "Animation/AnimMontage.h"
#include "Characters/GP_PlayerCharacter.h"
#include "GameplayTags/GP_Tags.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Utils/GP_BlueprintLibrary.h"

// 이 부분은 BP에도 관상용으로 만들어뒀으니 블루프린트 코드로 만들고싶으면 보셈 - 슝민
void UGP_Primary::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                  const FGameplayAbilityActivationInfo ActivationInfo,
                                  const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	{
		if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}

		// Sequence 1: Gameplay Event 대기 Task 생성 
		if (AttackEventTag.IsValid())
		{
			UAbilityTask_WaitGameplayEvent* WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
				this, AttackEventTag, nullptr, false, true);
			if (WaitEventTask)
			{
				// 콤보 한 번의 어빌리티 안에서 여러 공격 판정 Notify를 받을 수 있게 반복 이벤트로 둔다.
				WaitEventTask->EventReceived.AddDynamic(this, &ThisClass::OnAttackEventReceived);
				WaitEventTask->ReadyForActivation();
			}
		}

		UAnimMontage* MontageToPlay = AttackMontage;
		AGP_PlayerCharacter* PlayerCharacter = Cast<AGP_PlayerCharacter>(GetAvatarActorFromActorInfo());
		if (PlayerCharacter)
		{
			if (UAnimMontage* CharacterMontage = PlayerCharacter->StartPrimaryAttackCombo())
			{
				MontageToPlay = CharacterMontage;
			}
		}

		// Sequence 0: 몽타주 Task 생성
		if (MontageToPlay) // 캐릭터 세트 우선, 없으면 어빌리티 기본값 사용
		{
			if (!PlayPrimaryAttackMontage(MontageToPlay))
			{
				if (PlayerCharacter)
				{
					PlayerCharacter->CancelPrimaryAttackCombo();
				}

				EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			}
		}
		else // 몽타주가 비어있음 블루프린트 몽타주로
		{
			if (bDrawDebugs)
			{
				UE_LOG(LogTemp, Warning,
				       TEXT("Primary Attack: C++ Montage is NULL. Waiting for Blueprint to handle Montage."));
			}
		}
	}
}

bool UGP_Primary::PlayPrimaryAttackMontage(UAnimMontage* MontageToPlay)
{
	if (!IsValid(MontageToPlay))
	{
		return false;
	}

	UAbilityTask_PlayMontageAndWait* PlayMontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, MontageToPlay, 1.0f);
	if (!PlayMontageTask)
	{
		return false;
	}

	if (AGP_PlayerCharacter* PlayerCharacter = Cast<AGP_PlayerCharacter>(GetAvatarActorFromActorInfo()))
	{
		PlayerCharacter->SetPrimaryAttackActive(true);
	}

	// BlendOut이 아니라 실제 완료 시점에 콤보 유예 시간을 열어 애니메이션이 중간에 잘려 보이지 않게 한다.
	PlayMontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
	PlayMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	PlayMontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageInterrupted);

	PlayMontageTask->ReadyForActivation();
	return true;
}

void UGP_Primary::OnMontageCompleted()
{
	if (AGP_PlayerCharacter* PlayerCharacter = Cast<AGP_PlayerCharacter>(GetAvatarActorFromActorInfo()))
	{
		if (UAnimMontage* NextMontage = PlayerCharacter->AdvancePrimaryAttackCombo())
		{
			if (PlayPrimaryAttackMontage(NextMontage))
			{
				return;
			}
		}

		PlayerCharacter->FinishPrimaryAttackCombo();
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGP_Primary::OnMontageInterrupted()
{
	if (AGP_PlayerCharacter* PlayerCharacter = Cast<AGP_PlayerCharacter>(GetAvatarActorFromActorInfo()))
	{
		PlayerCharacter->CancelPrimaryAttackCombo();
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGP_Primary::OnAttackEventReceived(FGameplayEventData Payload)
{
	TArray<AActor*> HitActors = UGP_BlueprintLibrary::SphereMeleeHitBoxOverlap(
		GetAvatarActorFromActorInfo(), HitBoxRadius, HitBoxForwardOffset, HitBoxElevationOffset, bDrawDebugs);

	UGP_BlueprintLibrary::SendGameplayEventToActors(GetAvatarActorFromActorInfo(), HitActors,
	                                                GPTags::Events::Enemy::HitReact);

	if (HasAuthority(&CurrentActivationInfo))
	{
		UGP_BlueprintLibrary::ApplyGameplayEffectToActors(GetAvatarActorFromActorInfo(), HitActors, DamageEffectClass,
		                                                  GetAbilityLevel());
	}
}
