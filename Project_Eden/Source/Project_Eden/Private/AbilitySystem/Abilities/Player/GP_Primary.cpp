#include "AbilitySystem/Abilities/Player/GP_Primary.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/GP_AttributeSet.h"
#include "Animation/AnimMontage.h"
#include "Characters/GP_PlayerCharacter.h"
#include "GameplayTags/GP_Tags.h"

#include "Animation/PDA_CharacterAnimationSet.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Utils/GP_BlueprintLibrary.h"
#include "GameplayTask.h"


// 이 부분은 BP에도 관상용으로 만들어뒀으니 블루프린트 코드로 만들고싶으면 보셈 - 슝민
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

void UGP_Primary::StartComboSequence()
{
	bHasQueuedNextAttack = false;

	AGP_PlayerCharacter* PC = Cast<AGP_PlayerCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(PC))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// 변경점: GetAnimationSet()을 통해 직접 몽타주 배열 접근
	UAnimMontage* MontageToPlay = nullptr;
	if (UPDA_CharacterAnimationSet* AnimSet = PC->GetAnimationSet())
	{
		if (AnimSet->LightAttackMontages.IsValidIndex(CurrentComboIndex))
		{
			MontageToPlay = AnimSet->LightAttackMontages[CurrentComboIndex];
		}
	}

	if (!IsValid(MontageToPlay))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	// 3. 타격 이벤트(AttackHit) 대기 태스크 (버그 원인 해결!)
	UAbilityTask_WaitGameplayEvent* WaitHitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GPTags::Event::Player::AttackHit);
	WaitHitEventTask->EventReceived.AddDynamic(this, &ThisClass::OnAttackEventReceived);
	WaitHitEventTask->ReadyForActivation();

	// 4. 후딜레이 캔슬 및 다음 콤보 이행(ActionEnd) 대기 태스크
	UAbilityTask_WaitGameplayEvent* WaitEndEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GPTags::Event::Player::ActionEnd);
	WaitEndEventTask->EventReceived.AddDynamic(this, &ThisClass::OnActionEndEventReceived);
	WaitEndEventTask->ReadyForActivation();
	
	// 1. 몽타주 재생 태스크
	UAbilityTask_PlayMontageAndWait* PlayTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, MontageToPlay, 1.0f);
	PlayTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
	PlayTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	PlayTask->ReadyForActivation();

	// 2. 선입력 버퍼링 태스크 (몽타주 재생 중 추가 공격 키 입력 감지)
	UAbilityTask_WaitInputPress* InputTask = UAbilityTask_WaitInputPress::WaitInputPress(this, false);
	InputTask->OnPress.AddDynamic(this, &ThisClass::OnInputPressedDuringCombo);
	InputTask->ReadyForActivation();
}

void UGP_Primary::OnInputPressedDuringCombo(float TimeWaited)
{
	// 애니메이션 재생 중 공격 키가 다시 눌리면 다음 콤보 시퀀스를 예약
	bHasQueuedNextAttack = true;
}

bool UGP_Primary::PlayPrimaryAttackMontage(UAnimMontage* MontageToPlay)
{
	if (!IsValid(MontageToPlay)) return false;

	// Task를 사용하여 몽타주 재생 및 완료 대기 (Tick 제거)
	UAbilityTask_PlayMontageAndWait* PlayTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, MontageToPlay, 1.0f);
    
	if (!PlayTask) return false;

	// 완료/중단 시 후속 처리 바인딩
	PlayTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
	PlayTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
    
	PlayTask->ReadyForActivation();
	return true;
}

void UGP_Primary::OnMontageCompleted()
{
	if (bHasQueuedNextAttack)
	{
		// 예약된 공격이 있다면 인덱스를 올리고 콤보를 이어나감
		CurrentComboIndex++;
		StartComboSequence();
	}
	else
	{
		// 추가 입력이 없었다면 콤보 인덱스를 초기화하고 어빌리티 종료
		CurrentComboIndex = 0;
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UGP_Primary::OnMontageInterrupted()
{
	CurrentComboIndex = 0;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGP_Primary::OnAttackEventReceived(FGameplayEventData Payload)
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

// 액션 종료
void UGP_Primary::OnActionEndEventReceived(FGameplayEventData Payload)
{
	if (bHasQueuedNextAttack)
	{
		CurrentComboIndex++;
		StartComboSequence();
	}
	else
	{
		CurrentComboIndex = 0;
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}
