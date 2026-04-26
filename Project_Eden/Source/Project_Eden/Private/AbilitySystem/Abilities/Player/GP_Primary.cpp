#include "AbilitySystem/Abilities/Player/GP_Primary.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/GP_AttributeSet.h"
#include "Animation/AnimMontage.h"
#include "Characters/GP_PlayerCharacter.h"
#include "GameplayTags/GP_Tags.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "Utils/GP_BlueprintLibrary.h"

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

	// 캐릭터의 PDA(DataAsset) 등에서 현재 인덱스에 맞는 콤보 몽타주를 가져옴
	UAnimMontage* MontageToPlay = PC->GetPrimaryAttackMontageForStep(EGPPrimaryAttackType::Light, CurrentComboIndex);
	if (!IsValid(MontageToPlay))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

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
	                                                GPTags::Events::Enemy::HitReact);

	if (HasAuthority(&CurrentActivationInfo))
	{
		UGP_BlueprintLibrary::ApplyGameplayEffectToActors(GetAvatarActorFromActorInfo(), HitActors, DamageEffectClass,
		                                                  GetAbilityLevel());
	}
}
