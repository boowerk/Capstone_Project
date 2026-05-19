#include "AbilitySystem/Abilities/Enemy/GP_BossSweepAttack.h"

#include "Actors/GP_BossSweepTelegraphActor.h"
#include "AIController.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/Abilities/GP_GameplayAbility.h"
#include "Animation/AnimMontage.h"
#include "Animation/PDA_CharacterAnimationSet.h"
#include "Characters/GP_BaseCharacter.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffect.h"
#include "GameplayTags/GP_Tags.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Utils/GP_BlueprintLibrary.h"

namespace GP_BossSweepAttack_Internal
{
	UAnimMontage* ResolveSweepMontage(AActor* AvatarActor, UAnimMontage* ConfiguredMontage)
	{
		const AGP_BaseCharacter* BaseCharacter = Cast<AGP_BaseCharacter>(AvatarActor);
		const UPDA_CharacterAnimationSet* AnimationSet = IsValid(BaseCharacter) ? BaseCharacter->AnimationSet : nullptr;
		if (IsValid(AnimationSet) && AnimationSet->HeavyAttackMontages.Num() > 0 && IsValid(AnimationSet->HeavyAttackMontages[0]))
		{
			// Sans stores the requested sweep clip as the first heavy montage, mirroring the player combo data layout.
			return AnimationSet->HeavyAttackMontages[0];
		}

		return ConfiguredMontage;
	}
}

UGP_BossSweepAttack::UGP_BossSweepAttack()
{
	// Sweep attack uses a broad forward arc so dodging behind or far to the side remains a clear counterplay.
	FGameplayTagContainer AbilityAssetTags;
	AbilityAssetTags.AddTag(GPTags::Ability::Enemy::Attack_BossSweep);
	SetAssetTags(AbilityAssetTags);

	AttackRadius = 650.0f;
	ForwardOffset = 180.0f;
	bUseGameplayEventForHitTiming = false;
	bShowBossSweepTelegraph = true;
	BossSweepTelegraphDelay = 0.85f;
	BossSweepTelegraphColor = FLinearColor(1.0f, 0.08f, 0.02f, 0.48f);
	BossSweepArcAngleDegrees = 165.0f;
	BossSweepHitBoxElevationOffset = 40.0f;

	// Sans sweep animation is resolved from the boss AnimationSet so the setup commandlet can create the montage first.

	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageEffectFinder(TEXT("/Game/GAS_Pattern/AbilitySystem/GameplayEffects/GE_PrimaryDamage"));
	if (DamageEffectFinder.Succeeded())
	{
		DamageEffectClass = DamageEffectFinder.Class;
	}
}

void UGP_BossSweepAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	UGP_GameplayAbility::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bBossSweepHitApplied = false;
	bWaitingForBossSweepTelegraph = false;
	bBossSweepMontageStarted = false;
	ActiveBossSweepTelegraph = nullptr;
	PendingBossSweepMontage = nullptr;

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// The boss should commit to the sweep wind-up instead of sliding while the floor warning is visible.
	if (APawn* AvatarPawn = Cast<APawn>(AvatarActor))
	{
		if (AAIController* AIController = Cast<AAIController>(AvatarPawn->GetController()))
		{
			AIController->StopMovement();
		}
	}

	PendingBossSweepMontage = GP_BossSweepAttack_Internal::ResolveSweepMontage(AvatarActor, SkillMontage);
	if (!StartBossSweepTelegraph(AvatarActor))
	{
		BeginBossSweepAttack();
	}
}

void UGP_BossSweepAttack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BossSweepTelegraphTimerHandle);
	}

	if (IsValid(ActiveBossSweepTelegraph))
	{
		ActiveBossSweepTelegraph->Destroy();
		ActiveBossSweepTelegraph = nullptr;
	}

	bWaitingForBossSweepTelegraph = false;
	bBossSweepMontageStarted = false;
	PendingBossSweepMontage = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGP_BossSweepAttack::OnBossSweepMontageCompleted()
{
	if (!bBossSweepHitApplied)
	{
		PerformBossSweepHit();
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

bool UGP_BossSweepAttack::StartBossSweepTelegraph(AActor* AvatarActor)
{
	if (!bShowBossSweepTelegraph || BossSweepTelegraphDelay <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	SpawnBossSweepTelegraph(AvatarActor);
	bWaitingForBossSweepTelegraph = true;

	if (UWorld* World = GetWorld())
	{
		// The sweep animation and hit are delayed until the red floor warning has been readable for 0.85 seconds.
		World->GetTimerManager().SetTimer(
			BossSweepTelegraphTimerHandle,
			this,
			&ThisClass::OnBossSweepTelegraphElapsed,
			BossSweepTelegraphDelay,
			false);
		return true;
	}

	bWaitingForBossSweepTelegraph = false;
	return false;
}

void UGP_BossSweepAttack::OnBossSweepTelegraphElapsed()
{
	bWaitingForBossSweepTelegraph = false;

	if (IsValid(ActiveBossSweepTelegraph))
	{
		ActiveBossSweepTelegraph->Destroy();
		ActiveBossSweepTelegraph = nullptr;
	}

	BeginBossSweepAttack();
}

void UGP_BossSweepAttack::BeginBossSweepAttack()
{
	if (bBossSweepMontageStarted)
	{
		return;
	}

	bool bMontageTaskStarted = false;
	if (PendingBossSweepMontage)
	{
		UAbilityTask_PlayMontageAndWait* PlayMontageTask =
			UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, PendingBossSweepMontage, 1.0f);
		if (PlayMontageTask)
		{
			PlayMontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnBossSweepMontageCompleted);
			PlayMontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnBossSweepMontageCompleted);
			PlayMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnBossSweepMontageCompleted);
			PlayMontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnBossSweepMontageCompleted);
			PlayMontageTask->ReadyForActivation();
			bMontageTaskStarted = true;
			bBossSweepMontageStarted = true;
		}
	}

	// The damage window opens only after the warning has disappeared, matching the visible sweep start.
	PerformBossSweepHit();

	if (!bMontageTaskStarted)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UGP_BossSweepAttack::SpawnBossSweepTelegraph(AActor* AvatarActor)
{
	if (!IsValid(AvatarActor))
	{
		return;
	}

	UWorld* World = AvatarActor->GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = AvatarActor;
	SpawnParameters.Instigator = Cast<APawn>(AvatarActor);
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FVector SpawnLocation = AvatarActor->GetActorLocation();
	const FRotator SpawnRotation(0.0f, AvatarActor->GetActorRotation().Yaw, 0.0f);
	ActiveBossSweepTelegraph = World->SpawnActor<AGP_BossSweepTelegraphActor>(
		AGP_BossSweepTelegraphActor::StaticClass(),
		SpawnLocation,
		SpawnRotation,
		SpawnParameters);

	if (ActiveBossSweepTelegraph)
	{
		// The hitbox is offset forward, so the visible fan uses the full reach from the boss origin.
		const float TelegraphRadius = AttackRadius + FMath::Max(0.0f, ForwardOffset);
		ActiveBossSweepTelegraph->InitializeSweepTelegraph(
			TelegraphRadius,
			BossSweepArcAngleDegrees,
			BossSweepTelegraphDelay,
			BossSweepTelegraphColor,
			BossSweepTelegraphMaterial);
	}
}

void UGP_BossSweepAttack::PerformBossSweepHit()
{
	if (bBossSweepHitApplied)
	{
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor))
	{
		bBossSweepHitApplied = true;
		return;
	}

	// Sans sweep keeps its own forward-arc damage path instead of sharing the generic enemy attack timing.
	const TArray<AActor*> HitActors = UGP_BlueprintLibrary::ForwardArcMeleeHitBoxOverlap(
		AvatarActor,
		AttackRadius,
		ForwardOffset,
		BossSweepArcAngleDegrees,
		BossSweepHitBoxElevationOffset,
		bDrawDebugs);

	if (HasAuthority(&CurrentActivationInfo) && DamageEffectClass)
	{
		UGP_BlueprintLibrary::ApplyGameplayEffectToActors(
			AvatarActor,
			HitActors,
			DamageEffectClass,
			GetAbilityLevel());
	}

	bBossSweepHitApplied = true;
}
