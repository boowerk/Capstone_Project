// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/Player/Character1/GP_Skill_NetTestProjectile.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/Abilities/GP_SkillData.h"
#include "Animation/AnimMontage.h"
#include "Animation/GP_AnimNotify_SendGameplayEvent.h"
#include "Animation/PDA_CharacterAnimationSet.h"
#include "Actors/GP_Projectile.h"
#include "Characters/GP_PlayerCharacter.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Character.h"
#include "GameplayTags/GP_Tags.h"
#include "TimerManager.h"

namespace GP_NetTestProjectile_Internal
{
	float FindFirstGameplayEventNotifyTime(const UAnimMontage* Montage, const FGameplayTag& EventTag)
	{
		if (!IsValid(Montage) || !EventTag.IsValid())
		{
			return -1.0f;
		}

		for (const FAnimNotifyEvent& NotifyEvent : Montage->Notifies)
		{
			if (const UGP_AnimNotify_SendGameplayEvent* EventNotify = Cast<UGP_AnimNotify_SendGameplayEvent>(NotifyEvent.Notify))
			{
				if (EventNotify->GameplayEventTag == EventTag)
				{
					return NotifyEvent.GetTime();
				}
			}

			const FString NotifyName = NotifyEvent.NotifyName.ToString();
			if (NotifyName.Equals(EventTag.ToString(), ESearchCase::IgnoreCase)
				|| (EventTag.MatchesTagExact(GPTags::Event::Player::AttackHit) && NotifyName.Equals(TEXT("AttackHit"), ESearchCase::IgnoreCase)))
			{
				return NotifyEvent.GetTime();
			}
		}

		return -1.0f;
	}
}

UGP_Skill_NetTestProjectile::UGP_Skill_NetTestProjectile()
{
	SelectionMode = EGP_SkillSelectionMode::Projectile;
	bEndAbilityAfterConfirmedExecute = false;
}

void UGP_Skill_NetTestProjectile::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	ClearProjectileMontageState();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGP_Skill_NetTestProjectile::ExecuteConfirmedSkill_Implementation(const FGP_SkillTargetData& TargetData)
{
	AGP_PlayerCharacter* PlayerCharacter = Cast<AGP_PlayerCharacter>(GetAvatarActorFromActorInfo());
	UAnimMontage* MontageToPlay = SkillMontage;
	UAnimMontage* SourceMontageToPlay = nullptr;

	if (!IsValid(MontageToPlay) && IsValid(PlayerCharacter))
	{
		if (UPDA_CharacterAnimationSet* AnimSet = PlayerCharacter->GetAnimationSet())
		{
			SourceMontageToPlay = AnimSet->SourceSpellMontages.SimpleShoot;
		}
	}

	if (!IsValid(MontageToPlay) && !IsValid(SourceMontageToPlay))
	{
		SpawnProjectiles(TargetData);
		EndAfterProjectileMontage(false);
		return;
	}

	PendingTargetData = TargetData;
	bHasSpawnedProjectiles = false;

	if (IsValid(MontageToPlay))
	{
		WaitHitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GPTags::Event::Player::AttackHit);
		if (WaitHitTask)
		{
			WaitHitTask->EventReceived.AddDynamic(this, &ThisClass::OnAttackHitEventReceived);
			WaitHitTask->ReadyForActivation();
		}

		MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, MontageToPlay, 1.0f);
		if (!MontageTask)
		{
			SpawnProjectiles(TargetData);
			EndAfterProjectileMontage(false);
			return;
		}

		MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageFinished);
		MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageFinished);
		MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
		MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageInterrupted);
		MontageTask->ReadyForActivation();
		return;
	}

	if (!IsValid(PlayerCharacter))
	{
		SpawnProjectiles(TargetData);
		EndAfterProjectileMontage(false);
		return;
	}

	const float FallbackDuration = PlayerCharacter->PlayUEFNSourceFallbackMontage(SourceMontageToPlay, 1.0f);
	if (FallbackDuration <= 0.0f)
	{
		SpawnProjectiles(TargetData);
		EndAfterProjectileMontage(false);
		return;
	}

	bPlayingSourceFallbackMontage = true;

	if (UWorld* World = GetWorld())
	{
		const float AttackHitTime = GP_NetTestProjectile_Internal::FindFirstGameplayEventNotifyTime(SourceMontageToPlay, GPTags::Event::Player::AttackHit);
		if (AttackHitTime >= 0.0f && AttackHitTime < FallbackDuration)
		{
			World->GetTimerManager().SetTimer(
				FallbackAttackHitTimerHandle,
				this,
				&ThisClass::OnFallbackAttackHit,
				AttackHitTime,
				false);
		}

		World->GetTimerManager().SetTimer(
			FallbackMontageEndTimerHandle,
			this,
			&ThisClass::OnFallbackMontageFinished,
			FallbackDuration,
			false);
	}
}

void UGP_Skill_NetTestProjectile::SpawnProjectiles(const FGP_SkillTargetData& TargetData)
{
	ACharacter* Avatar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* ASC = CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;
	UGP_SkillData* SkillData = GetSkillDataFromSpec(CurrentSpecHandle, CurrentActorInfo);
	const TSubclassOf<AActor> SpawnActorClass = GetSkillSpawnActorClass(SkillData, ProjectileClass);

	if (!Avatar || !ASC || !SpawnActorClass)
	{
		return;
	}

	if (HasAuthority(&CurrentActivationInfo) && SpawnActorClass)
	{
		const FGameplayTag TechElementTag = GetCurrentTechElementTag(CurrentActorInfo);
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Avatar;
		SpawnParams.Instigator = Avatar;
		SpawnParams.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		const int32 SafeProjectileCount = FMath::Max(1, 1 + GetSkillAugmentProjectileCountBonus(SkillData, CurrentActorInfo));
		const float BonusSpreadAngle = FMath::Max(SkillData->ProjectileSpreadAngle, 0.0f);
		const float StepAngle = SafeProjectileCount > 1
			? BonusSpreadAngle / static_cast<float>(SafeProjectileCount - 1)
			: 0.0f;
		const float StartYawOffset = -BonusSpreadAngle * 0.5f;

		for (int32 Index = 0; Index < SafeProjectileCount; ++Index)
		{
			FRotator SpawnRotation = TargetData.AimDirection.Rotation();
			SpawnRotation.Pitch = FMath::ClampAngle(SpawnRotation.Pitch, -5.f, 10.f);
			SpawnRotation.Yaw += StartYawOffset + StepAngle * Index;

			const FVector AimDirection = SpawnRotation.Vector();
			const FVector SpawnLocation =
				Avatar->GetActorLocation()
				+ AimDirection * SpawnForwardOffset
				+ FVector::UpVector * SpawnUpOffset;

			AActor* SpawnedActor = Avatar->GetWorld()->SpawnActor<AActor>(
				SpawnActorClass,
				SpawnLocation,
				SpawnRotation,
				SpawnParams
			);
			AGP_Projectile* Projectile = Cast<AGP_Projectile>(SpawnedActor);

			if (Projectile)
			{
				Projectile->SetSkillData(SkillData);
				Projectile->SetProjectileVisualSystem(GetProjectileVisualSystem(SkillData, TechElementTag));
				Projectile->ApplySplashRadiusMultiplier(
					GetSkillAugmentRadiusMultiplier(SkillData, CurrentActorInfo));
				Projectile->SetImpactVisualActorClass(GetSkillVisualActorClass(
					SkillData,
					nullptr,
					TechElementTag,
					GPTags::Ability::Skill::Visual::Impact));
			}
			else if (SpawnedActor)
			{
				UE_LOG(LogTemp, Warning, TEXT("%s expected AGP_Projectile but spawned %s."), *GetNameSafe(this), *GetNameSafe(SpawnedActor));
			}
		}
	}
}

void UGP_Skill_NetTestProjectile::EndAfterProjectileMontage(bool bWasCancelled)
{
	const bool bReplicateEndAbility = bWasCancelled || HasAuthority(&CurrentActivationInfo);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGP_Skill_NetTestProjectile::OnAttackHitEventReceived(FGameplayEventData Payload)
{
	if (bHasSpawnedProjectiles)
	{
		return;
	}

	bHasSpawnedProjectiles = true;
	SpawnProjectiles(PendingTargetData);
}

void UGP_Skill_NetTestProjectile::OnMontageFinished()
{
	if (!bHasSpawnedProjectiles)
	{
		bHasSpawnedProjectiles = true;
		SpawnProjectiles(PendingTargetData);
	}

	EndAfterProjectileMontage(false);
}

void UGP_Skill_NetTestProjectile::OnMontageInterrupted()
{
	EndAfterProjectileMontage(true);
}

void UGP_Skill_NetTestProjectile::OnFallbackAttackHit()
{
	if (bHasSpawnedProjectiles)
	{
		return;
	}

	bHasSpawnedProjectiles = true;
	SpawnProjectiles(PendingTargetData);
}

void UGP_Skill_NetTestProjectile::OnFallbackMontageFinished()
{
	if (!bHasSpawnedProjectiles)
	{
		bHasSpawnedProjectiles = true;
		SpawnProjectiles(PendingTargetData);
	}

	EndAfterProjectileMontage(false);
}

void UGP_Skill_NetTestProjectile::ClearProjectileMontageState()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FallbackAttackHitTimerHandle);
		World->GetTimerManager().ClearTimer(FallbackMontageEndTimerHandle);
	}

	if (bPlayingSourceFallbackMontage)
	{
		if (AGP_PlayerCharacter* PlayerCharacter = Cast<AGP_PlayerCharacter>(GetAvatarActorFromActorInfo()))
		{
			PlayerCharacter->StopUEFNSourceFallbackMontage(0.2f);
		}
	}

	if (MontageTask)
	{
		MontageTask->EndTask();
		MontageTask = nullptr;
	}

	if (WaitHitTask)
	{
		WaitHitTask->EndTask();
		WaitHitTask = nullptr;
	}

	bPlayingSourceFallbackMontage = false;
}
