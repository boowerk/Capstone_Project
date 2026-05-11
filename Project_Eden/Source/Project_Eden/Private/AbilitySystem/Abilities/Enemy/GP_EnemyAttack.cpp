#include "AbilitySystem/Abilities/Enemy/GP_EnemyAttack.h"
#include "AIController.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffect.h"
#include "GameplayTags/GP_Tags.h"

UGP_EnemyAttack::UGP_EnemyAttack()
{
	// 공유 BT 공격 태스크는 이 태그를 기준으로 적 공격 어빌리티를 찾는다.
	FGameplayTagContainer AbilityAssetTags;
	AbilityAssetTags.AddTag(GPTags::Ability::Enemy::Attack_Melee);
	SetAssetTags(AbilityAssetTags);

	AttackEventTag = GPTags::Event::Enemy::AttackHit;

	// 부모 클래스 수치 기본값 설정
	AttackRadius = 120.0f;
	ForwardOffset = 140.0f;
}

void UGP_EnemyAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bHasAppliedAttackHit = false;

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 공격 시작 시 이동을 잠깐 멈춰 몽타주와 타격 방향이 어긋나지 않도록 한다.
	if (APawn* AvatarPawn = Cast<APawn>(AvatarActor))
	{
		if (AAIController* AIController = Cast<AAIController>(AvatarPawn->GetController()))
		{
			AIController->StopMovement();
		}
	}

	// 부모의 SkillMontage 변수를 공격 몽타주로 사용
	UAnimMontage* MontageToPlay = SkillMontage;

	if (MontageToPlay)
	{
		if (bUseGameplayEventForHitTiming && AttackEventTag.IsValid())
		{
			UAbilityTask_WaitGameplayEvent* WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
				this, AttackEventTag, nullptr, true, true);
			if (WaitEventTask)
			{
				WaitEventTask->EventReceived.AddDynamic(this, &ThisClass::OnAttackEventReceived);
				WaitEventTask->ReadyForActivation();
			}
		}

		UAbilityTask_PlayMontageAndWait* PlayMontageTask =
			UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, MontageToPlay, 1.0f);
		if (PlayMontageTask)
		{
			PlayMontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
			PlayMontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageCompleted);
			PlayMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageCompleted);
			PlayMontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCompleted);
			PlayMontageTask->ReadyForActivation();
		}

		// 이벤트 타이밍을 쓰지 않는 경우 어빌리티 시작 즉시 판정을 수행
		if (!bUseGameplayEventForHitTiming || !AttackEventTag.IsValid())
		{
			PerformAttackHit();
		}

		return;
	}

	// 몽타주가 없으면 즉시 판정 후 종료
	PerformAttackHit();
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGP_EnemyAttack::OnMontageCompleted()
{
	// 이벤트 노티파이를 빠뜨린 경우에도 최소한의 공격 판정은 보장
	if (!bHasAppliedAttackHit)
	{
		PerformAttackHit();
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGP_EnemyAttack::OnAttackEventReceived(FGameplayEventData Payload)
{
	PerformAttackHit();
}

void UGP_EnemyAttack::PerformAttackHit()
{
	if (bHasAppliedAttackHit) return;

	// Generic enemies keep the shared spherical hit; boss-only shapes live in derived boss ability classes.
	PerformAreaAttack();

	bHasAppliedAttackHit = true;
}
