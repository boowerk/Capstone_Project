#include "AbilitySystem/Abilities/Player/GP_Dash.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Characters/GP_PlayerCharacter.h"
#include "GameplayTags/GP_Tags.h"
#include "Animation/PDA_CharacterAnimationSet.h"

UGP_Dash::UGP_Dash()
{
	// 어빌리티 고유 식별 태그
	AbilityTags.AddTag(GPTags::Ability::Movement::Dash);
    
	// 대시 어빌리티가 실행 중(Active)일 때 캐릭터에게 부여되는 상태 태그들
	ActivationOwnedTags.AddTag(GPTags::State::Movement::Dash);       // 대시 상태 적용
	ActivationOwnedTags.AddTag(GPTags::State::Status::Unstoppable);  // 필요시 저지불가 적용
	// ActivationOwnedTags.AddTag(GPTags::State::Status::Invincible); // 필요시 무적 판정 적용
}

void UGP_Dash::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// [Rule: Early Return]
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

	// 변경점: GetAnimationSet()을 통해 몽타주 접근
	UAnimMontage* RollMontage = nullptr;
	if (UPDA_CharacterAnimationSet* AnimSet = PC->GetAnimationSet())
	{
		RollMontage = AnimSet->RollMontage;
	}

	if (!IsValid(RollMontage))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 캐릭터 회전 로직 유지
	FVector RollDirection = PC->GetLastMovementInputVector();
	if (RollDirection.IsNearlyZero())
	{
		RollDirection = PC->GetActorForwardVector();
	}
	RollDirection.Z = 0.0f;
	RollDirection.Normalize();
	PC->SetActorRotation(RollDirection.Rotation());

	// 몽타주 실행 태스크 생성
	UAbilityTask_PlayMontageAndWait* PlayTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, RollMontage, 1.0f);

	// 콜백 바인딩을 통해 틱(Tick) 없이 몽타주 종료를 감지
	PlayTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
	PlayTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	PlayTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageInterrupted);

	PlayTask->ReadyForActivation();
}

void UGP_Dash::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGP_Dash::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
