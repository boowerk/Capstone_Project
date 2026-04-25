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
void UGP_Primary::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// [Rule: Early Return]
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 캐릭터의 하드코딩된 로직 대신 어빌리티의 DataAsset 몽타주 우선 활용
	UAnimMontage* MontageToPlay = AttackMontage;
    
	// 캐릭터에 설정된 애니메이션 세트가 있다면 가져오되, 실행 제어는 GA가 담당
	if (AGP_PlayerCharacter* PC = Cast<AGP_PlayerCharacter>(GetAvatarActorFromActorInfo()))
	{
		if (UAnimMontage* ComboMontage = PC->GetPrimaryAttackMontage())
		{
			MontageToPlay = ComboMontage;
		}
	}

	if (!PlayPrimaryAttackMontage(MontageToPlay))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
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
