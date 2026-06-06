#include "AbilitySystem/Abilities/GP_TargetedSkillBase.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Characters/GP_BaseCharacter.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameplayTags/GP_Tags.h"
#include "Components/PrimitiveComponent.h"
#include "TimerManager.h"
#include "Utils/GP_BlueprintLibrary.h"

UGP_TargetedSkillBase::UGP_TargetedSkillBase()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer AbilityAssetTags;
	AbilityAssetTags.AddTag(GPTags::Ability::Skill::SkillRoot);
	AbilityAssetTags.AddTag(GPTags::Ability::Skill::Selection);
	SetAssetTags(AbilityAssetTags);

	ActivationOwnedTags.AddTag(GPTags::State::Skill::Selecting);
}

void UGP_TargetedSkillBase::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid() || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (SelectionMode == EGP_SkillSelectionMode::Instant)
	{
		FGP_SkillTargetData TargetData = GetCurrentTargetData(EGP_SkillConfirmType::Primary);
		if (!TryCommitAndExecute(TargetData))
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		}
		return;
	}

	BeginSelection();
}

void UGP_TargetedSkillBase::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	CleanupSelection();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGP_TargetedSkillBase::ExecuteConfirmedSkill_Implementation(const FGP_SkillTargetData& TargetData)
{
	UE_LOG(LogTemp, Warning, TEXT("Targeted skill %s confirmed at %s with no ExecuteConfirmedSkill override."),
		*GetName(),
		*TargetData.TargetLocation.ToCompactString());
}

FGP_SkillTargetData UGP_TargetedSkillBase::GetCurrentTargetData(EGP_SkillConfirmType ConfirmType) const
{
	FGP_SkillTargetData TargetData;
	TargetData.SelectionMode = SelectionMode;
	TargetData.ConfirmType = ConfirmType;

	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor))
	{
		return TargetData;
	}

	FRotator AimRotation = AvatarActor->GetActorRotation();
	if (const APawn* Pawn = Cast<APawn>(AvatarActor))
	{
		if (const AController* Controller = Pawn->GetController())
		{
			AimRotation = Controller->GetControlRotation();
		}
	}

	TargetData.Origin = AvatarActor->GetActorLocation();
	TargetData.AimDirection = AimRotation.Vector().GetSafeNormal();
	if (TargetData.AimDirection.IsNearlyZero())
	{
		TargetData.AimDirection = AvatarActor->GetActorForwardVector();
	}

	const FVector TraceStart = TargetData.Origin;
	const FVector TraceEnd = TraceStart + TargetData.AimDirection * MaxTargetRange;
	TargetData.TargetLocation = TraceEnd;

	if (SelectionMode == EGP_SkillSelectionMode::Projectile)
	{
		return TargetData;
	}

	UWorld* World = AvatarActor->GetWorld();
	if (!World)
	{
		return TargetData;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GP_TargetedSkillTrace), false);
	QueryParams.AddIgnoredActor(AvatarActor);

	if (SelectionMode == EGP_SkillSelectionMode::TargetActor && bUseAimAssistTargetSelection)
	{
		if (AActor* BestTarget = FindBestTargetActor(AvatarActor, TraceStart, TargetData.AimDirection))
		{
			TargetData.TargetActor = BestTarget;
			TargetData.TargetLocation = BestTarget->GetActorLocation();
			TargetData.bBlockingHit = true;
			return TargetData;
		}
	}

	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, TargetTraceChannel, QueryParams);
	if (!bHit)
	{
		return TargetData;
	}

	TargetData.bBlockingHit = true;
	TargetData.TargetLocation = Hit.ImpactPoint;

	if (SelectionMode == EGP_SkillSelectionMode::TargetActor)
	{
		AActor* HitActor = Hit.GetActor();
		if (IsValid(HitActor) && (!TargetActorClassFilter || HitActor->IsA(TargetActorClassFilter)))
		{
			TargetData.TargetActor = HitActor;
		}
	}

	return TargetData;
}

AActor* UGP_TargetedSkillBase::FindBestTargetActor(const AActor* AvatarActor, const FVector& TraceStart, const FVector& AimDirection) const
{
	if (!IsValid(AvatarActor))
	{
		return nullptr;
	}

	UWorld* World = AvatarActor->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GP_TargetedSkillTargetOverlap), false);
	QueryParams.AddIgnoredActor(AvatarActor);

	FCollisionResponseParams ResponseParams;
	ResponseParams.CollisionResponse.SetAllChannels(ECR_Ignore);
	ResponseParams.CollisionResponse.SetResponse(ECC_Pawn, ECR_Block);

	TArray<FOverlapResult> OverlapResults;
	const FCollisionShape SelectionShape = FCollisionShape::MakeSphere(MaxTargetRange);
	World->OverlapMultiByChannel(OverlapResults, TraceStart, FQuat::Identity, ECC_Visibility, SelectionShape, QueryParams, ResponseParams);

	AActor* BestTarget = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	const FVector SafeAimDirection = AimDirection.GetSafeNormal();
	const float MaxPerpendicularDistance = FMath::Max(TargetAimAssistRadius, 1.0f);

	for (const FOverlapResult& Result : OverlapResults)
	{
		AActor* Candidate = Result.GetActor();
		if (!IsValid(Candidate) || Candidate == AvatarActor)
		{
			continue;
		}

		if (TargetActorClassFilter && !Candidate->IsA(TargetActorClassFilter))
		{
			continue;
		}

		if (!TargetActorClassFilter && !Candidate->IsA<AGP_BaseCharacter>())
		{
			continue;
		}

		if (bTargetSelectionRequiresCombatEffect && !UGP_BlueprintLibrary::CanApplyCombatEffect(const_cast<AActor*>(AvatarActor), Candidate))
		{
			continue;
		}

		if (!HasSelectionLineOfSight(AvatarActor, Candidate))
		{
			continue;
		}

		const FVector ToCandidate = Candidate->GetActorLocation() - TraceStart;
		const float AlongAim = FVector::DotProduct(ToCandidate, SafeAimDirection);
		if (AlongAim < 0.0f || AlongAim > MaxTargetRange)
		{
			continue;
		}

		const FVector ClosestPointOnAim = TraceStart + SafeAimDirection * AlongAim;
		const float PerpendicularDistance = FVector::Dist(Candidate->GetActorLocation(), ClosestPointOnAim);
		if (PerpendicularDistance > MaxPerpendicularDistance)
		{
			continue;
		}

		const float Score = PerpendicularDistance + AlongAim * 0.05f;
		if (Score < BestScore)
		{
			BestScore = Score;
			BestTarget = Candidate;
		}
	}

	return BestTarget;
}

bool UGP_TargetedSkillBase::HasSelectionLineOfSight(const AActor* AvatarActor, const AActor* CandidateActor) const
{
	if (!IsValid(AvatarActor) || !IsValid(CandidateActor))
	{
		return false;
	}

	UWorld* World = AvatarActor->GetWorld();
	if (!World)
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GP_TargetedSkillLineOfSight), false);
	QueryParams.AddIgnoredActor(AvatarActor);
	QueryParams.AddIgnoredActor(CandidateActor);

	const FVector Start = AvatarActor->GetActorLocation() + FVector::UpVector * 60.0f;
	const FVector End = CandidateActor->GetActorLocation() + FVector::UpVector * 60.0f;

	TArray<FHitResult> Hits;
	World->LineTraceMultiByChannel(Hits, Start, End, TargetTraceChannel, QueryParams);
	for (const FHitResult& Hit : Hits)
	{
		if (IsActorBlockingSkillLineOfSight(Hit.GetActor()))
		{
			return false;
		}
	}

	return true;
}

bool UGP_TargetedSkillBase::IsActorBlockingSkillLineOfSight(const AActor* HitActor) const
{
	if (!IsValid(HitActor))
	{
		return false;
	}

	if (HitActor->IsA<APawn>())
	{
		return false;
	}

	const UPrimitiveComponent* HitPrimitive = Cast<UPrimitiveComponent>(HitActor->GetRootComponent());
	if (HitPrimitive && (HitPrimitive->IsSimulatingPhysics() || HitPrimitive->Mobility != EComponentMobility::Static))
	{
		return false;
	}

	return true;
}

void UGP_TargetedSkillBase::BeginSelection()
{
	if (bSelectionActive)
	{
		return;
	}

	bSelectionActive = true;
	AddSelectionLooseTags();
	RegisterSelectionEvents();

	if (PreviewActorClass && IsLocallyControlled())
	{
		if (AActor* AvatarActor = GetAvatarActorFromActorInfo())
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = AvatarActor;
			SpawnParams.Instigator = Cast<APawn>(AvatarActor);
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			PreviewActor = AvatarActor->GetWorld()->SpawnActor<AActor>(
				PreviewActorClass,
				AvatarActor->GetActorLocation(),
				AvatarActor->GetActorRotation(),
				SpawnParams);
		}
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(PreviewTimerHandle, this, &ThisClass::UpdatePreview, PreviewUpdateInterval, true, 0.0f);
	}
}

void UGP_TargetedSkillBase::CleanupSelection()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PreviewTimerHandle);
	}

	if (IsValid(PreviewActor))
	{
		PreviewActor->Destroy();
	}

	PreviewActor = nullptr;
	PrimaryConfirmTask = nullptr;
	SecondaryConfirmTask = nullptr;
	CancelTask = nullptr;
	RemoveSelectionLooseTags();
	bSelectionActive = false;
}

void UGP_TargetedSkillBase::RegisterSelectionEvents()
{
	PrimaryConfirmTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GPTags::Event::Skill::ConfirmPrimary, nullptr, false, false);
	if (PrimaryConfirmTask)
	{
		PrimaryConfirmTask->EventReceived.AddDynamic(this, &ThisClass::OnPrimaryConfirm);
		PrimaryConfirmTask->ReadyForActivation();
	}

	if (bAllowSecondaryConfirm)
	{
		SecondaryConfirmTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GPTags::Event::Skill::ConfirmSecondary, nullptr, false, false);
		if (SecondaryConfirmTask)
		{
			SecondaryConfirmTask->EventReceived.AddDynamic(this, &ThisClass::OnSecondaryConfirm);
			SecondaryConfirmTask->ReadyForActivation();
		}
	}

	CancelTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GPTags::Event::Skill::Cancel, nullptr, false, false);
	if (CancelTask)
	{
		CancelTask->EventReceived.AddDynamic(this, &ThisClass::OnCancelSelection);
		CancelTask->ReadyForActivation();
	}
}

void UGP_TargetedSkillBase::AddSelectionLooseTags()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	AddedLooseTags.Reset();
	AddedLooseTags.AddTag(GPTags::State::Skill::Previewing);

	switch (SelectionMode)
	{
	case EGP_SkillSelectionMode::Projectile:
		AddedLooseTags.AddTag(GPTags::State::Skill::Projectile);
		break;
	case EGP_SkillSelectionMode::Ray:
		AddedLooseTags.AddTag(GPTags::State::Skill::Ray);
		break;
	case EGP_SkillSelectionMode::TargetActor:
		AddedLooseTags.AddTag(GPTags::State::Skill::TargetActor);
		break;
	default:
		break;
	}

	ASC->AddLooseGameplayTags(AddedLooseTags);
}

void UGP_TargetedSkillBase::RemoveSelectionLooseTags()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC || AddedLooseTags.IsEmpty())
	{
		return;
	}

	ASC->RemoveLooseGameplayTags(AddedLooseTags);
	AddedLooseTags.Reset();
}

void UGP_TargetedSkillBase::UpdatePreview()
{
	const FGP_SkillTargetData TargetData = GetCurrentTargetData();
	const FRotator PreviewRotation = TargetData.AimDirection.Rotation();

	if (IsValid(PreviewActor))
	{
		PreviewActor->SetActorLocationAndRotation(TargetData.TargetLocation, PreviewRotation);
	}

	if (!bDrawSelectionDebug && !bDrawDebugs)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FColor DebugColor = TargetData.bBlockingHit ? FColor::Green : FColor::Yellow;
	DrawDebugLine(World, TargetData.Origin, TargetData.TargetLocation, DebugColor, false, PreviewUpdateInterval, 0, 2.0f);
	DrawDebugSphere(World, TargetData.TargetLocation, 20.0f, 12, DebugColor, false, PreviewUpdateInterval);
}

void UGP_TargetedSkillBase::ConfirmSelection(EGP_SkillConfirmType ConfirmType)
{
	if (!bSelectionActive)
	{
		return;
	}

	FGP_SkillTargetData TargetData = GetCurrentTargetData(ConfirmType);
	if (!TryCommitAndExecute(TargetData))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

bool UGP_TargetedSkillBase::TryCommitAndExecute(const FGP_SkillTargetData& TargetData)
{
	if (bRequireBlockingHit && !TargetData.bBlockingHit)
	{
		UE_LOG(LogTemp, Warning, TEXT("Targeted skill %s rejected confirm because no blocking hit was found."), *GetName());
		return false;
	}

	if (SelectionMode == EGP_SkillSelectionMode::TargetActor && bRequireBlockingHit && !IsValid(TargetData.TargetActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("Targeted skill %s rejected confirm because no valid target actor was found."), *GetName());
		return false;
	}

	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		return false;
	}

	CleanupSelection();
	ExecuteConfirmedSkill(TargetData);

	if (bEndAbilityAfterConfirmedExecute)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}

	return true;
}

void UGP_TargetedSkillBase::OnPrimaryConfirm(FGameplayEventData Payload)
{
	ConfirmSelection(EGP_SkillConfirmType::Primary);
}

void UGP_TargetedSkillBase::OnSecondaryConfirm(FGameplayEventData Payload)
{
	ConfirmSelection(EGP_SkillConfirmType::Secondary);
}

void UGP_TargetedSkillBase::OnCancelSelection(FGameplayEventData Payload)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
