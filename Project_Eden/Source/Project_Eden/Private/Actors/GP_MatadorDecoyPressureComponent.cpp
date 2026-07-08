#include "Actors/GP_MatadorDecoyPressureComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Characters/GP_MatadorBossStateComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTags/GP_Tags.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Utils/GP_BlueprintLibrary.h"

UGP_MatadorDecoyPressureComponent::UGP_MatadorDecoyPressureComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UGP_MatadorDecoyPressureComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoStartPressure && HasAuthority())
	{
		StartPressure();
	}
}

void UGP_MatadorDecoyPressureComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!HasAuthority() || !bPressureActive)
	{
		return;
	}

	if (!IsPressureAllowed())
	{
		PressureState = EGPMatadorDecoyPressureState::Inactive;
		return;
	}

	if (bPostTeleportAttackLocked && GetWorld() && GetWorld()->GetTimeSeconds() >= PostTeleportUnlockTime)
	{
		bPostTeleportAttackLocked = false;
		PressureState = EGPMatadorDecoyPressureState::WalkApproach;
	}

	if (!IsTargetValid(CurrentTarget.Get()))
	{
		SetCurrentTarget(SelectRandomTarget());
	}

	if (!IsTargetValid(CurrentTarget.Get()))
	{
		PressureState = EGPMatadorDecoyPressureState::SelectTarget;
		return;
	}

	UpdateDistanceAndTeleportRequest(DeltaTime);
	UpdateTeleportFlow();

	if (!bTeleportRequested && !bPatternActionLocked && !bPostTeleportAttackLocked)
	{
		UpdateWalkApproach(DeltaTime);
	}
}

void UGP_MatadorDecoyPressureComponent::InitializePressure(AActor* InMainBossActor, UGP_MatadorBossStateComponent* InStateComponent)
{
	MainBossActor = InMainBossActor;
	MatadorStateComponent = InStateComponent;
}

void UGP_MatadorDecoyPressureComponent::StartPressure()
{
	if (!HasAuthority())
	{
		return;
	}

	bPressureActive = true;
	PressureState = EGPMatadorDecoyPressureState::SelectTarget;
	SetCurrentTarget(SelectRandomTarget());
}

void UGP_MatadorDecoyPressureComponent::StopPressure()
{
	if (!HasAuthority())
	{
		return;
	}

	bPressureActive = false;
	bTeleportRequested = false;
	bPatternActionLocked = false;
	bPostTeleportAttackLocked = false;
	OutOfRangeElapsed = 0.0f;
	PressureState = EGPMatadorDecoyPressureState::Inactive;
}

void UGP_MatadorDecoyPressureComponent::RequestTeleport(bool bAdvanceComboOnTeleport)
{
	if (!HasAuthority() || bTeleportRequested)
	{
		return;
	}

	bTeleportRequested = true;
	bAdvanceComboWhenTeleporting = bAdvanceComboOnTeleport;
	PressureState = bPatternActionLocked
		? EGPMatadorDecoyPressureState::TeleportReserved
		: EGPMatadorDecoyPressureState::PreTeleportGrace;

	if (!bPatternActionLocked && GetWorld())
	{
		PreTeleportReadyTime = GetWorld()->GetTimeSeconds() + FMath::Max(0.0f, PreTeleportGraceTime);
	}
}

void UGP_MatadorDecoyPressureComponent::NotifyPatternActionStarted()
{
	if (!HasAuthority())
	{
		return;
	}

	bPatternActionLocked = true;
	PressureState = EGPMatadorDecoyPressureState::PatternActionLocked;
}

void UGP_MatadorDecoyPressureComponent::NotifyPatternActionFinished()
{
	if (!HasAuthority())
	{
		return;
	}

	bPatternActionLocked = false;

	if (bTeleportRequested)
	{
		PressureState = EGPMatadorDecoyPressureState::PreTeleportGrace;
		if (GetWorld())
		{
			PreTeleportReadyTime = GetWorld()->GetTimeSeconds() + FMath::Max(0.0f, PreTeleportGraceTime);
		}
		return;
	}

	PressureState = bPostTeleportAttackLocked
		? EGPMatadorDecoyPressureState::PostTeleportGrace
		: EGPMatadorDecoyPressureState::WalkApproach;
}

void UGP_MatadorDecoyPressureComponent::AdvanceStepThrustIndex()
{
	if (!HasAuthority())
	{
		return;
	}

	StepThrustIndex = FMath::Clamp(StepThrustIndex + 1, 0, FMath::Max(0, MaxNormalStepThrustCount));
}

bool UGP_MatadorDecoyPressureComponent::HasAuthority() const
{
	const AActor* Owner = GetOwner();
	return IsValid(Owner) && Owner->HasAuthority();
}

bool UGP_MatadorDecoyPressureComponent::IsPressureAllowed() const
{
	const UGP_MatadorBossStateComponent* StateComponent = MatadorStateComponent.Get();
	return !IsValid(StateComponent) || !StateComponent->IsGroggy();
}

bool UGP_MatadorDecoyPressureComponent::IsTargetValid(AActor* Candidate) const
{
	if (!IsValid(Candidate) || Candidate->IsActorBeingDestroyed())
	{
		return false;
	}

	const APawn* CandidatePawn = Cast<APawn>(Candidate);
	if (!IsValid(CandidatePawn) || !CandidatePawn->IsPlayerControlled())
	{
		return false;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Candidate);
	if (IsValid(TargetASC) && TargetASC->HasMatchingGameplayTag(GPTags::State::Status::Invincible))
	{
		return false;
	}

	if (AActor* SourceActor = MainBossActor.Get())
	{
		return UGP_BlueprintLibrary::CanApplyCombatEffect(SourceActor, Candidate);
	}

	return true;
}

AActor* UGP_MatadorDecoyPressureComponent::SelectRandomTarget()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	TArray<AActor*> Candidates;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PlayerController = It->Get();
		APawn* PlayerPawn = IsValid(PlayerController) ? PlayerController->GetPawn() : nullptr;
		if (IsTargetValid(PlayerPawn))
		{
			Candidates.Add(PlayerPawn);
		}
	}

	if (Candidates.IsEmpty())
	{
		return nullptr;
	}

	if (bAvoidImmediateSameTarget && Candidates.Num() > 1)
	{
		Candidates.Remove(PreviousTarget.Get());
	}

	return Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
}

void UGP_MatadorDecoyPressureComponent::SetCurrentTarget(AActor* NewTarget)
{
	if (CurrentTarget.Get() == NewTarget)
	{
		return;
	}

	PreviousTarget = CurrentTarget;
	CurrentTarget = NewTarget;
	OutOfRangeElapsed = 0.0f;
	OnTargetChanged.Broadcast(NewTarget);
}

void UGP_MatadorDecoyPressureComponent::UpdateWalkApproach(float DeltaTime)
{
	AActor* Owner = GetOwner();
	AActor* Target = CurrentTarget.Get();
	if (!IsValid(Owner) || !IsValid(Target))
	{
		return;
	}

	FVector ToTarget = Target->GetActorLocation() - Owner->GetActorLocation();
	ToTarget.Z = 0.0f;
	const float Distance = ToTarget.Size2D();
	if (Distance <= FMath::Max(0.0f, MeleeEnterRange) || ToTarget.IsNearlyZero())
	{
		PressureState = EGPMatadorDecoyPressureState::WalkApproach;
		return;
	}

	const FVector Direction = ToTarget.GetSafeNormal2D();
	const float StepDistance = FMath::Min(Distance - MeleeEnterRange, FMath::Max(0.0f, WalkSpeed) * FMath::Max(0.0f, DeltaTime));
	const FVector NewLocation = Owner->GetActorLocation() + Direction * StepDistance;
	Owner->SetActorLocationAndRotation(NewLocation, ResolveFacingRotation(NewLocation, Target->GetActorLocation()), true, nullptr, ETeleportType::None);
	PressureState = EGPMatadorDecoyPressureState::WalkApproach;
}

void UGP_MatadorDecoyPressureComponent::UpdateDistanceAndTeleportRequest(float DeltaTime)
{
	if (bTeleportRequested || bPostTeleportAttackLocked)
	{
		return;
	}

	const AActor* Owner = GetOwner();
	const AActor* Target = CurrentTarget.Get();
	if (!IsValid(Owner) || !IsValid(Target))
	{
		return;
	}

	const float Distance = FVector::Dist2D(Owner->GetActorLocation(), Target->GetActorLocation());
	if (Distance > FMath::Max(MeleeExitRange, MeleeEnterRange))
	{
		OutOfRangeElapsed += FMath::Max(0.0f, DeltaTime);
		if (OutOfRangeElapsed >= FMath::Max(0.0f, TargetOutOfRangeTime))
		{
			RequestTeleport(true);
		}
	}
	else if (Distance <= FMath::Max(0.0f, MeleeEnterRange))
	{
		OutOfRangeElapsed = 0.0f;
	}
}

void UGP_MatadorDecoyPressureComponent::UpdateTeleportFlow()
{
	if (!bTeleportRequested || bPatternActionLocked || !GetWorld())
	{
		return;
	}

	if (PressureState == EGPMatadorDecoyPressureState::TeleportReserved)
	{
		PressureState = EGPMatadorDecoyPressureState::PreTeleportGrace;
		PreTeleportReadyTime = GetWorld()->GetTimeSeconds() + FMath::Max(0.0f, PreTeleportGraceTime);
	}

	if (GetWorld()->GetTimeSeconds() >= PreTeleportReadyTime)
	{
		ExecuteReservedTeleport();
	}
}

void UGP_MatadorDecoyPressureComponent::ExecuteReservedTeleport()
{
	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}

	AActor* NewTarget = CurrentTarget.Get();
	if (!IsTargetValid(NewTarget))
	{
		NewTarget = SelectRandomTarget();
	}
	if (!IsTargetValid(NewTarget))
	{
		bTeleportRequested = false;
		PressureState = EGPMatadorDecoyPressureState::SelectTarget;
		return;
	}

	FVector TeleportLocation = FVector::ZeroVector;
	if (!ResolveTeleportLocation(NewTarget, TeleportLocation))
	{
		bTeleportRequested = false;
		return;
	}

	if (bAdvanceComboWhenTeleporting)
	{
		AdvanceStepThrustIndex();
	}

	SetCurrentTarget(NewTarget);
	Owner->SetActorLocationAndRotation(TeleportLocation, ResolveFacingRotation(TeleportLocation, NewTarget->GetActorLocation()), false, nullptr, ETeleportType::TeleportPhysics);

	bTeleportRequested = false;
	bPostTeleportAttackLocked = true;
	OutOfRangeElapsed = 0.0f;
	PressureState = EGPMatadorDecoyPressureState::PostTeleportGrace;
	PostTeleportUnlockTime = GetWorld() ? GetWorld()->GetTimeSeconds() + FMath::Max(0.0f, PostTeleportGraceTime) : 0.0f;
	OnTeleportExecuted.Broadcast(NewTarget, TeleportLocation);
}

bool UGP_MatadorDecoyPressureComponent::ResolveTeleportLocation(AActor* TargetActor, FVector& OutLocation) const
{
	if (!IsValid(TargetActor))
	{
		return false;
	}

	const float OwnerCapsuleHalfHeight = GetOwnerCapsuleHalfHeight();
	const float MinDistance = FMath::Max(0.0f, TeleportMinDistance);
	const float PreferredDistance = FMath::Clamp(TeleportPreferredDistance, MinDistance, FMath::Max(MinDistance, TeleportMaxDistance));
	const float MaxDistance = FMath::Max(PreferredDistance, TeleportMaxDistance);
	const FVector TargetLocation = TargetActor->GetActorLocation();
	FVector TargetForward = TargetActor->GetActorForwardVector().GetSafeNormal2D();
	if (TargetForward.IsNearlyZero())
	{
		TargetForward = FVector::ForwardVector;
	}

	const float CandidateYawOffsets[] = { 35.0f, -35.0f, 70.0f, -70.0f, 0.0f, 110.0f, -110.0f, 180.0f };
	const float CandidateDistances[] = { PreferredDistance, MinDistance, MaxDistance };
	const int32 YawStartIndex = FMath::RandRange(0, UE_ARRAY_COUNT(CandidateYawOffsets) - 1);
	const int32 DistanceStartIndex = FMath::RandRange(0, UE_ARRAY_COUNT(CandidateDistances) - 1);

	for (int32 DistanceOffset = 0; DistanceOffset < UE_ARRAY_COUNT(CandidateDistances); ++DistanceOffset)
	{
		const float Distance = CandidateDistances[(DistanceStartIndex + DistanceOffset) % UE_ARRAY_COUNT(CandidateDistances)];
		for (int32 YawIndex = 0; YawIndex < UE_ARRAY_COUNT(CandidateYawOffsets); ++YawIndex)
		{
			const float YawOffset = CandidateYawOffsets[(YawStartIndex + YawIndex) % UE_ARRAY_COUNT(CandidateYawOffsets)];
			const FVector Direction = FRotator(0.0f, YawOffset, 0.0f).RotateVector(TargetForward).GetSafeNormal2D();
			FVector CandidateLocation = TargetLocation + Direction * Distance;
			if (UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld()))
			{
				FNavLocation ProjectedLocation;
				if (!NavigationSystem->ProjectPointToNavigation(CandidateLocation, ProjectedLocation, FVector(240.0f, 240.0f, 320.0f)))
				{
					continue;
				}
				CandidateLocation = ProjectedLocation.Location;
				CandidateLocation.Z += OwnerCapsuleHalfHeight;
			}

			const float ActualDistance = FVector::Dist2D(TargetLocation, CandidateLocation);
			if (ActualDistance >= MinDistance && ActualDistance <= MaxDistance)
			{
				OutLocation = CandidateLocation;
				return true;
			}
		}
	}

	OutLocation = TargetLocation - TargetForward * PreferredDistance;
	return true;
}

float UGP_MatadorDecoyPressureComponent::GetOwnerCapsuleHalfHeight() const
{
	const AActor* Owner = GetOwner();
	const UCapsuleComponent* Capsule = IsValid(Owner) ? Owner->FindComponentByClass<UCapsuleComponent>() : nullptr;
	return IsValid(Capsule) ? Capsule->GetScaledCapsuleHalfHeight() : 0.0f;
}

FRotator UGP_MatadorDecoyPressureComponent::ResolveFacingRotation(const FVector& FromLocation, const FVector& ToLocation) const
{
	FVector LookDirection = ToLocation - FromLocation;
	LookDirection.Z = 0.0f;
	if (LookDirection.IsNearlyZero())
	{
		return FRotator::ZeroRotator;
	}
	return LookDirection.Rotation();
}
