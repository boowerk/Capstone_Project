#include "Animation/GP_MatadorDecoyAnimInstance.h"

#include "Animation/AnimMontage.h"
#include "Actors/GP_MatadorBossDecoyActor.h"
#include "UObject/ConstructorHelpers.h"

UGP_MatadorDecoyAnimInstance::UGP_MatadorDecoyAnimInstance()
{
	static ConstructorHelpers::FObjectFinder<UAnimMontage> RapierThrustMontageFinder(TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_Matador/Animation/AM_MatadorDecoy_RapierThrust.AM_MatadorDecoy_RapierThrust"));
	if (RapierThrustMontageFinder.Succeeded())
	{
		RapierThrustMontage = RapierThrustMontageFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimMontage> CapeGustMontageFinder(TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_Matador/Animation/AM_MatadorDecoy_CapeGust.AM_MatadorDecoy_CapeGust"));
	if (CapeGustMontageFinder.Succeeded())
	{
		CapeGustMontage = CapeGustMontageFinder.Object;
	}
}

void UGP_MatadorDecoyAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	CacheDecoyOwner();
}

void UGP_MatadorDecoyAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	CacheDecoyOwner();
	PresentationStateTime += FMath::Max(0.0f, DeltaSeconds);
	if (PresentationState != EGPMatadorDecoyPresentationState::Idle
		&& PresentationStateDuration > 0.0f
		&& PresentationStateTime >= PresentationStateDuration)
	{
		SetPresentationState(EGPMatadorDecoyPresentationState::Idle, PresentationDirection, 0.0f);
	}

	AActor* OwnerActor = DecoyOwner.Get();
	if (!IsValid(OwnerActor))
	{
		GroundSpeed = 0.0f;
		bIsWalkingPressure = false;
		PressureState = EGPMatadorDecoyPressureState::Inactive;
		CurrentTarget = nullptr;
		bTeleportRequested = false;
		bPostTeleportAttackLocked = false;
		StepThrustIndex = 0;
		bHasPreviousOwnerLocation = false;
		SetPresentationState(EGPMatadorDecoyPresentationState::Idle, FVector::ForwardVector, 0.0f);
		return;
	}

	const FVector OwnerLocation = OwnerActor->GetActorLocation();
	if (bHasPreviousOwnerLocation && DeltaSeconds > KINDA_SMALL_NUMBER)
	{
		const FVector DeltaLocation = OwnerLocation - PreviousOwnerLocation;
		GroundSpeed = FVector(DeltaLocation.X, DeltaLocation.Y, 0.0f).Size() / DeltaSeconds;
	}
	else
	{
		GroundSpeed = OwnerActor->GetVelocity().Size2D();
		bHasPreviousOwnerLocation = true;
	}
	PreviousOwnerLocation = OwnerLocation;

	if (UGP_MatadorDecoyPressureComponent* CachedPressureComponent = PressureComponent.Get())
	{
		PressureState = CachedPressureComponent->GetPressureState();
		CurrentTarget = CachedPressureComponent->GetCurrentTarget();
		bTeleportRequested = CachedPressureComponent->IsTeleportRequested();
		bPostTeleportAttackLocked = CachedPressureComponent->IsPostTeleportAttackLocked();
		StepThrustIndex = CachedPressureComponent->GetStepThrustIndex();
	}

	// The AnimBP already blends Idle/Walk from this flag.  Restricting it to the
	// pressure approach state left normal follow/return movement visually idle.
	// Teleports must stay idle because their transform jump can report a large speed.
	bIsWalkingPressure = GroundSpeed > 3.0f && !bTeleportRequested;
}

void UGP_MatadorDecoyAnimInstance::NotifyRapierAimStarted(float InAimDuration)
{
	SetPresentationState(EGPMatadorDecoyPresentationState::RapierAim, PresentationDirection, FMath::Max(0.0f, InAimDuration));
}

void UGP_MatadorDecoyAnimInstance::NotifyRapierDirectionLocked(FVector LockedDirection, float InCommitDelay)
{
	SetPresentationState(EGPMatadorDecoyPresentationState::RapierLocked, LockedDirection, FMath::Max(0.0f, InCommitDelay));
}

void UGP_MatadorDecoyAnimInstance::NotifyRapierThrust(FVector LockedDirection)
{
	SetPresentationState(EGPMatadorDecoyPresentationState::RapierThrust, LockedDirection, 0.35f);
	PlayPresentationMontage(RapierThrustMontage, RapierThrustMontagePlayRate);
}

void UGP_MatadorDecoyAnimInstance::NotifyCapePrepareStarted(float InPrepareDuration)
{
	SetPresentationState(EGPMatadorDecoyPresentationState::CapePrepare, PresentationDirection, FMath::Max(0.0f, InPrepareDuration));
}

void UGP_MatadorDecoyAnimInstance::NotifyCapeDirectionLocked(FVector LockedDirection)
{
	SetPresentationState(EGPMatadorDecoyPresentationState::CapeLocked, LockedDirection, 0.2f);
}

void UGP_MatadorDecoyAnimInstance::NotifyCapeGustBurst(FVector LockedDirection, int32 InBurstIndex, int32 InBurstCount)
{
	CapeBurstIndex = InBurstIndex;
	CapeBurstCount = InBurstCount;
	SetPresentationState(EGPMatadorDecoyPresentationState::CapeGust, LockedDirection, 0.25f);
	if (bPlayCapeGustMontageOnEveryBurst || InBurstIndex <= 0 || !Montage_IsPlaying(CapeGustMontage))
	{
		PlayPresentationMontage(CapeGustMontage, CapeGustMontagePlayRate);
	}
}

void UGP_MatadorDecoyAnimInstance::CacheDecoyOwner()
{
	if (IsValid(DecoyOwner))
	{
		return;
	}

	DecoyOwner = Cast<AGP_MatadorBossDecoyActor>(GetOwningActor());
	PressureComponent = IsValid(DecoyOwner) ? DecoyOwner->GetPressureComponent() : nullptr;
}

void UGP_MatadorDecoyAnimInstance::SetPresentationState(EGPMatadorDecoyPresentationState NewState, FVector Direction, float Duration)
{
	PresentationState = NewState;
	PresentationStateDuration = FMath::Max(0.0f, Duration);
	PresentationStateTime = 0.0f;

	FVector SafeDirection = Direction.GetSafeNormal2D();
	if (SafeDirection.IsNearlyZero())
	{
		if (AActor* OwnerActor = DecoyOwner.Get())
		{
			SafeDirection = OwnerActor->GetActorForwardVector().GetSafeNormal2D();
		}
	}
	PresentationDirection = SafeDirection.IsNearlyZero() ? FVector::ForwardVector : SafeDirection;
	RefreshPresentationFlags();
}

void UGP_MatadorDecoyAnimInstance::RefreshPresentationFlags()
{
	bRapierAimActive = PresentationState == EGPMatadorDecoyPresentationState::RapierAim;
	bRapierLockedActive = PresentationState == EGPMatadorDecoyPresentationState::RapierLocked;
	bRapierThrustActive = PresentationState == EGPMatadorDecoyPresentationState::RapierThrust;
	bCapePrepareActive = PresentationState == EGPMatadorDecoyPresentationState::CapePrepare;
	bCapeLockedActive = PresentationState == EGPMatadorDecoyPresentationState::CapeLocked;
	bCapeGustActive = PresentationState == EGPMatadorDecoyPresentationState::CapeGust;
}

void UGP_MatadorDecoyAnimInstance::PlayPresentationMontage(UAnimMontage* Montage, float PlayRate)
{
	if (!IsValid(Montage))
	{
		return;
	}

	Montage_Play(Montage, FMath::Max(0.01f, PlayRate));
}
