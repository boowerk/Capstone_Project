#include "AbilitySystem/Abilities/Enemy/GP_EnemyAttack.h"
#include "AIController.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/PDA_CharacterAnimationSet.h"
#include "Animation/PDA_EnemyAnimationSet.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Characters/GP_EnemyCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffect.h"
#include "GameplayTags/GP_Tags.h"
#include "UObject/ConstructorHelpers.h"
#include "Utils/GP_BlueprintLibrary.h"

namespace GP_EnemyAttack_Internal
{
	int8 ResolveMontageSide(const UAnimMontage* Montage)
	{
		const FString MontageName = GetNameSafe(Montage);
		if (MontageName.EndsWith(TEXT("_L")))
		{
			return -1;
		}
		if (MontageName.EndsWith(TEXT("_R")))
		{
			return 1;
		}
		return 0;
	}

	UAnimMontage* ResolveAttackMontage(
		AActor* AvatarActor,
		UAnimMontage* ConfiguredMontage,
		TObjectPtr<UAnimMontage>& LastSelectedAttackMontage,
		int8& LastSelectedAttackSide)
	{
		if (IsValid(ConfiguredMontage))
		{
			return ConfiguredMontage;
		}

		const AGP_EnemyCharacter* EnemyCharacter = Cast<AGP_EnemyCharacter>(AvatarActor);
		if (!IsValid(EnemyCharacter))
		{
			return nullptr;
		}

		TArray<UAnimMontage*> Candidates;
		if (const UPDA_EnemyAnimationSet* EnemyAnimationSet = EnemyCharacter->GetEnemyAnimationSet())
		{
			for (UAnimMontage* Montage : EnemyAnimationSet->LightAttackMontages)
			{
				if (IsValid(Montage))
				{
					Candidates.Add(Montage);
				}
			}

			if (Candidates.IsEmpty())
			{
				return EnemyAnimationSet->PrimaryAttackMontage.Get();
			}
		}
		else
		{
			// Keep existing bosses operational while their animation data is migrated incrementally.
			const UPDA_CharacterAnimationSet* LegacyAnimationSet = EnemyCharacter->AnimationSet;
			if (!IsValid(LegacyAnimationSet))
			{
				return nullptr;
			}

			for (UAnimMontage* Montage : LegacyAnimationSet->LightAttackMontages)
			{
				if (IsValid(Montage))
				{
					Candidates.Add(Montage);
				}
			}

			if (Candidates.IsEmpty())
			{
				return LegacyAnimationSet->PrimaryAttackMontage.Get();
			}
		}

		if (Candidates.Num() == 1)
		{
			LastSelectedAttackMontage = Candidates[0];
			LastSelectedAttackSide = ResolveMontageSide(Candidates[0]);
			return Candidates[0];
		}

		TArray<UAnimMontage*> OppositeSideCandidates;
		if (LastSelectedAttackSide != 0)
		{
			for (UAnimMontage* Montage : Candidates)
			{
				if (ResolveMontageSide(Montage) == -LastSelectedAttackSide)
				{
					OppositeSideCandidates.Add(Montage);
				}
			}
		}

		TArray<UAnimMontage*> NonRepeatingCandidates;
		for (UAnimMontage* Montage : Candidates)
		{
			if (Montage != LastSelectedAttackMontage)
			{
				NonRepeatingCandidates.Add(Montage);
			}
		}

		const TArray<UAnimMontage*>& SelectionPool = !OppositeSideCandidates.IsEmpty()
			? OppositeSideCandidates
			: (NonRepeatingCandidates.IsEmpty() ? Candidates : NonRepeatingCandidates);
		UAnimMontage* SelectedMontage = SelectionPool[FMath::RandHelper(SelectionPool.Num())];
		LastSelectedAttackMontage = SelectedMontage;
		LastSelectedAttackSide = ResolveMontageSide(SelectedMontage);
		return SelectedMontage;
	}

	UAnimSequence* ResolveLowerBodyRootMotionSequence(AActor* AvatarActor, const UAnimMontage* SelectedAttackMontage)
	{
		const AGP_EnemyCharacter* EnemyCharacter = Cast<AGP_EnemyCharacter>(AvatarActor);
		const UPDA_EnemyAnimationSet* AnimationSet = IsValid(EnemyCharacter) ? EnemyCharacter->GetEnemyAnimationSet() : nullptr;
		if (!IsValid(AnimationSet) || !IsValid(SelectedAttackMontage))
		{
			return nullptr;
		}

		const int32 AttackIndex = AnimationSet->LightAttackMontages.IndexOfByKey(const_cast<UAnimMontage*>(SelectedAttackMontage));
		return AnimationSet->LowerBodyRootMotionSequences.IsValidIndex(AttackIndex)
			? AnimationSet->LowerBodyRootMotionSequences[AttackIndex].Get()
			: nullptr;
	}
}

UGP_EnemyAttack::UGP_EnemyAttack()
{
	// 공유 BT 공격 태스크는 이 태그를 기준으로 적 공격 어빌리티를 찾는다.
	FGameplayTagContainer AbilityAssetTags;
	AbilityAssetTags.AddTag(GPTags::Ability::Enemy::Attack_Melee);
	SetAssetTags(AbilityAssetTags);

	AttackEventTag = GPTags::Event::Enemy::AttackHit;
	ActionEndEventTag = GPTags::Event::Enemy::ActionEnd;

	// 부모 클래스 수치 기본값 설정
	AttackRadius = 96.0f;
	ForwardOffset = 112.0f;

	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageEffectFinder(TEXT("/Game/GAS_Pattern/AbilitySystem/GameplayEffects/Damage/GE_PrimaryDamage"));
	if (DamageEffectFinder.Succeeded())
	{
		DamageEffectClass = DamageEffectFinder.Class;
	}
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
	bHasFinishedAttackAbility = false;

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (AGP_EnemyCharacter* EnemyCharacter = Cast<AGP_EnemyCharacter>(AvatarActor))
	{
		// Setting the attack lock also clears an active turn slot before DefaultSlot starts.
		EnemyCharacter->SetBasicEnemyAttackInProgress(true);
		if (const UPDA_EnemyAnimationSet* Set = EnemyCharacter->GetEnemyAnimationSet(); IsValid(Set) && Set->bUseAbilityForwardStep)
		{
			AbilityForwardStepDirection = AvatarActor->GetActorForwardVector().GetSafeNormal2D();
			AbilityForwardStepDistance = Set->AbilityForwardStepDistance;
			AbilityForwardStepDurationSeconds = FMath::Max(Set->AbilityForwardStepDurationSeconds, KINDA_SMALL_NUMBER);
			AbilityForwardStepPushSpeed = Set->AbilityForwardStepPushSpeed;
			if (AbilityForwardStepDistance > KINDA_SMALL_NUMBER && !AbilityForwardStepDirection.IsNearlyZero())
			{
				if (UAbilityTask_WaitDelay* Task = UAbilityTask_WaitDelay::WaitDelay(this, FMath::Max(0.0f, Set->AbilityForwardStepDelaySeconds)))
				{
					Task->OnFinish.AddDynamic(this, &ThisClass::BeginAbilityForwardStep);
					Task->ReadyForActivation();
				}
			}
		}
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
	UAnimMontage* MontageToPlay = GP_EnemyAttack_Internal::ResolveAttackMontage(
		AvatarActor,
		SkillMontage,
		LastSelectedAttackMontage,
		LastSelectedAttackSide);

	if (MontageToPlay)
	{
		UAnimMontage* LowerBodyMontage = nullptr;
		if (UAnimSequence* LowerBodyRootMotionSequence = GP_EnemyAttack_Internal::ResolveLowerBodyRootMotionSequence(AvatarActor, MontageToPlay))
		{
			LowerBodyMontage = UAnimMontage::CreateSlotAnimationAsDynamicMontage(
				LowerBodyRootMotionSequence,
				TEXT("Enemy_LowerBody"),
				0.1f,
				0.2f,
				1.0f,
				1);
		}

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

		if (ActionEndEventTag.IsValid())
		{
			UAbilityTask_WaitGameplayEvent* WaitActionEndTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
				this, ActionEndEventTag, nullptr, true, true);
			if (WaitActionEndTask)
			{
				WaitActionEndTask->EventReceived.AddDynamic(this, &ThisClass::OnActionEndEventReceived);
				WaitActionEndTask->ReadyForActivation();
			}
		}

		UAbilityTask_PlayMontageAndWait* PlayMontageTask =
			UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, MontageToPlay, 1.0f);
		if (PlayMontageTask)
		{
			PlayMontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
			PlayMontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageBlendOut);
			PlayMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageCancelled);
			PlayMontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCancelled);
			PlayMontageTask->ReadyForActivation();

			// GAS owns DefaultSlot so its replicated attack montage cannot be replaced.
			// The lower slot is visual-only and must not overwrite GAS's single current montage.
			if (LowerBodyMontage)
			{
				if (UAnimInstance* AnimInstance = GetCurrentActorInfo()->GetAnimInstance())
				{
					AnimInstance->Montage_Play(LowerBodyMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, false);
				}
			}
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
	FinishAttackAbility(false);
}

void UGP_EnemyAttack::OnMontageCompleted()
{
	if (ShouldFinishAttackForPresentationSignal(EEnemyAttackPresentationSignal::MontageCompleted))
	{
		FinishAttackAbility(false);
	}
}

void UGP_EnemyAttack::OnMontageBlendOut()
{
	// Blend-out is still visible presentation. Ending the ability here makes
	// PlayMontageAndWait stop the remaining blend and produces a visible pop.
	ensure(!ShouldFinishAttackForPresentationSignal(EEnemyAttackPresentationSignal::MontageBlendOut));
}

void UGP_EnemyAttack::OnMontageCancelled()
{
	// 사망·그로기·BT 안전 타임아웃으로 끊긴 몽타주는 아직 오지 않은 타격 notify를 보정하지 않는다.
	if (ShouldFinishAttackForPresentationSignal(EEnemyAttackPresentationSignal::MontageInterrupted))
	{
		FinishAttackAbility(true);
	}
}

void UGP_EnemyAttack::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	EndAbilityForwardStep();
	if (AGP_EnemyCharacter* EnemyCharacter = Cast<AGP_EnemyCharacter>(GetAvatarActorFromActorInfo()))
	{
		EnemyCharacter->SetBasicEnemyAttackInProgress(false);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGP_EnemyAttack::OnAttackEventReceived(FGameplayEventData Payload)
{
	PerformAttackHit();
	// AttackHit is authored at the strike/recovery boundary. Keep advancing through
	// the preparation and strike, then stop before the recoil animation begins.
	EndAbilityForwardStep();
}

void UGP_EnemyAttack::OnActionEndEventReceived(FGameplayEventData Payload)
{
	// ActionEnd closes the damaging/movement window, but the montage remains
	// ability-owned until its authored recovery and blend-out finish naturally.
	EndAbilityForwardStep();
	ensure(!ShouldFinishAttackForPresentationSignal(EEnemyAttackPresentationSignal::ActionEnd));
}

bool UGP_EnemyAttack::ShouldFinishAttackForPresentationSignal(EEnemyAttackPresentationSignal Signal)
{
	return Signal == EEnemyAttackPresentationSignal::MontageCompleted
		|| Signal == EEnemyAttackPresentationSignal::MontageInterrupted;
}

void UGP_EnemyAttack::FinishAttackAbility(bool bWasCancelled)
{
	if (bHasFinishedAttackAbility)
	{
		return;
	}

	bHasFinishedAttackAbility = true;

	// 이벤트 노티파이를 빠뜨린 경우에도 최소한의 공격 판정은 보장
	// 정상 종료에서만 누락 notify를 보정하고, 명시적으로 취소된 공격은 추가 피해 없이 끝낸다.
	if (ShouldApplyFallbackHit(bHasAppliedAttackHit, bWasCancelled))
	{
		PerformAttackHit();
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
}

bool UGP_EnemyAttack::ShouldApplyFallbackHit(bool bAlreadyApplied, bool bWasCancelled)
{
	return !bAlreadyApplied && !bWasCancelled;
}

void UGP_EnemyAttack::BeginAbilityForwardStep()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	AbilityForwardStepMovementComponent = IsValid(Character) ? Character->GetCharacterMovement() : nullptr;
	if (bHasFinishedAttackAbility || !IsValid(AbilityForwardStepMovementComponent) || AbilityForwardStepDirection.IsNearlyZero()) return;
	AbilityForwardStepCapsuleComponent = Character->GetCapsuleComponent();
	if (IsValid(AbilityForwardStepCapsuleComponent))
	{
		AbilityForwardStepPreviousPawnResponse = AbilityForwardStepCapsuleComponent->GetCollisionResponseToChannel(ECC_Pawn);
		AbilityForwardStepCapsuleComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		AbilityForwardStepCapsuleComponent->SetGenerateOverlapEvents(true);
	}
	AbilityForwardStepPushedActors.Reset();
	bAbilityForwardStepActive = true;
	TickAbilityForwardStep();
	GetWorld()->GetTimerManager().SetTimer(AbilityForwardStepTimerHandle, this, &ThisClass::TickAbilityForwardStep, 0.02f, true);
}

void UGP_EnemyAttack::TickAbilityForwardStep()
{
	if (!bAbilityForwardStepActive || !IsValid(AbilityForwardStepMovementComponent)) { EndAbilityForwardStep(); return; }
	AbilityForwardStepMovementComponent->RequestDirectMove(AbilityForwardStepDirection * (AbilityForwardStepDistance / AbilityForwardStepDurationSeconds), false);
	PushOverlappingForwardStepTargets();
}

void UGP_EnemyAttack::EndAbilityForwardStep()
{
	if (!bAbilityForwardStepActive) return;
	bAbilityForwardStepActive = false;
	if (IsValid(AbilityForwardStepMovementComponent)) AbilityForwardStepMovementComponent->StopMovementImmediately();
	if (IsValid(AbilityForwardStepCapsuleComponent))
	{
		AbilityForwardStepCapsuleComponent->SetCollisionResponseToChannel(ECC_Pawn, AbilityForwardStepPreviousPawnResponse);
	}
	AbilityForwardStepCapsuleComponent = nullptr;
	AbilityForwardStepPushedActors.Reset();
	if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(AbilityForwardStepTimerHandle);
}

void UGP_EnemyAttack::PushOverlappingForwardStepTargets()
{
	ACharacter* SourceCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(SourceCharacter) || !SourceCharacter->HasAuthority() || !IsValid(AbilityForwardStepCapsuleComponent)) return;

	TArray<AActor*> OverlappingActors;
	AbilityForwardStepCapsuleComponent->GetOverlappingActors(OverlappingActors, ACharacter::StaticClass());
	for (AActor* TargetActor : OverlappingActors)
	{
		ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor);
		if (!IsValid(TargetCharacter) || TargetCharacter == SourceCharacter || AbilityForwardStepPushedActors.Contains(TargetCharacter)) continue;
		if (!UGP_BlueprintLibrary::CanApplyCombatEffect(SourceCharacter, TargetCharacter)) continue;

		TargetCharacter->LaunchCharacter(AbilityForwardStepDirection * FMath::Max(0.0f, AbilityForwardStepPushSpeed), true, false);
		AbilityForwardStepPushedActors.Add(TargetCharacter);
	}
}

void UGP_EnemyAttack::PerformAttackHit()
{
	if (bHasAppliedAttackHit) return;

	// Generic enemies keep the shared spherical hit; boss-only shapes live in derived boss ability classes.
	PerformAreaAttack();

	bHasAppliedAttackHit = true;
}
