#include "AbilitySystem/Abilities/Player/GP_Dash.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Characters/GP_PlayerCharacter.h"
#include "GameplayTags/GP_Tags.h"
#include "Animation/PDA_CharacterAnimationSet.h"

UGP_Dash::UGP_Dash()
{
	// 어빌리티 고유 식별 태그
	AbilityTags.AddTag(GPTags::Ability::Movement::Dash);
	ActivationOwnedTags.AddTag(GPTags::State::Movement::Dash);       // 대시 상태 
	// ActivationOwnedTags.AddTag(GPTags::State::Status::Unstoppable);	// 저지불가 상태
	// ActivationOwnedTags.AddTag(GPTags::State::Status::Invincible);	// 무적 상태

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

	// 애니메이션 세트 - 대시 몽타주
	UAnimMontage* DashMontage = nullptr;
	if (UPDA_CharacterAnimationSet* AnimSet = PC->GetAnimationSet())
	{
		DashMontage = AnimSet->DashMontage;
	}

	if (!IsValid(DashMontage))
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
	
	DashDirection.Z = 0.0f; // Z축(위아래) 회전 방지
	
	if (!DashDirection.IsNearlyZero())
	{
		DashDirection.Normalize();
		PC->SetActorRotation(DashDirection.Rotation());
	}

	// 몽타주 실행 태스크 생성
	UAbilityTask_PlayMontageAndWait* PlayTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, DashMontage, 1.0f);

	if (PlayTask)
	{
		PlayTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
		PlayTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
		PlayTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageInterrupted);

		// 이제 빨간줄 없이 깔끔하게 인식됩니다.
		PlayTask->ReadyForActivation();
	}
	else
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

void UGP_Dash::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGP_Dash::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
