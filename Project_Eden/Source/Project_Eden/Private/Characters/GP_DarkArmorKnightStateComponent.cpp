#include "Characters/GP_DarkArmorKnightStateComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Characters/GP_DarkArmorKnightBossCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTags/GP_Tags.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

UGP_DarkArmorKnightStateComponent::UGP_DarkArmorKnightStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UGP_DarkArmorKnightStateComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GuardTimerHandle);
		World->GetTimerManager().ClearTimer(ParryTimerHandle);
		World->GetTimerManager().ClearTimer(GroggyTimerHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void UGP_DarkArmorKnightStateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGP_DarkArmorKnightStateComponent, GuardGauge);
	DOREPLIFETIME(UGP_DarkArmorKnightStateComponent, MaxGuardGauge);
	DOREPLIFETIME(UGP_DarkArmorKnightStateComponent, bIsGuarding);
	DOREPLIFETIME(UGP_DarkArmorKnightStateComponent, bParryWindowOpen);
	DOREPLIFETIME(UGP_DarkArmorKnightStateComponent, bGuardBroken);
	DOREPLIFETIME(UGP_DarkArmorKnightStateComponent, bIsGroggy);
	DOREPLIFETIME(UGP_DarkArmorKnightStateComponent, LastHitDirection);
}

void UGP_DarkArmorKnightStateComponent::InitializeDarkKnightState(AGP_DarkArmorKnightBossCharacter* InBoss)
{
	BossOwner = InBoss;
	if (IsValid(BossOwner) && IsValid(BossOwner->GetCharacterMovement()))
	{
		UnguardedMaxWalkSpeed = BossOwner->GetCharacterMovement()->MaxWalkSpeed;
	}
	if (GetOwnerRole() == ROLE_Authority)
	{
		SetCombatPhase(IsValid(BossOwner) ? BossOwner->GetDarkKnightPhase() : 1);
		GuardGauge = MaxGuardGauge;
		bGuardBroken = false;
		ApplyStateTags();
		OnGuardGaugeChanged.Broadcast(GuardGauge, MaxGuardGauge);
	}
}

bool UGP_DarkArmorKnightStateComponent::StartGuardStance(float Duration)
{
	if (GetOwnerRole() != ROLE_Authority || bIsGroggy || bGuardBroken)
	{
		return false;
	}

	SetGuardingInternal(true);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(GuardTimerHandle, this, &ThisClass::EndGuardStance, FMath::Max(0.01f, Duration), false);
	}
	return true;
}

void UGP_DarkArmorKnightStateComponent::EndGuardStance()
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	SetParryWindowInternal(false);
	SetGuardingInternal(false);
}

bool UGP_DarkArmorKnightStateComponent::StartParryWindow(float Duration)
{
	if (GetOwnerRole() != ROLE_Authority || !bIsGuarding || bIsGroggy || bGuardBroken)
	{
		return false;
	}

	SetParryWindowInternal(true);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			ParryTimerHandle,
			[this]() { SetParryWindowInternal(false); },
			FMath::Max(0.01f, Duration),
			false);
	}
	return true;
}

void UGP_DarkArmorKnightStateComponent::EnterGroggy(float Duration)
{
	if (GetOwnerRole() != ROLE_Authority || bIsGroggy)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GuardTimerHandle);
		World->GetTimerManager().ClearTimer(ParryTimerHandle);
		World->GetTimerManager().SetTimer(GroggyTimerHandle, this, &ThisClass::RecoverFromGroggy, FMath::Max(0.01f, Duration), false);
	}

	bGuardBroken = true;
	SetParryWindowInternal(false);
	SetGuardingInternal(false);
	SetGroggyInternal(true);
	ApplyStateTags();
}

void UGP_DarkArmorKnightStateComponent::RecoverFromGroggy()
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	bGuardBroken = false;
	SetGuardGaugeInternal(MaxGuardGauge);
	SetGroggyInternal(false);
	SetParryWindowInternal(false);
	SetGuardingInternal(false);
	ApplyStateTags();
}

float UGP_DarkArmorKnightStateComponent::ResolveIncomingDamageMultiplier(AActor* DamageInstigator, bool bHeavyAttack)
{
	if (bIsGroggy)
	{
		return GroggyDamageMultiplier;
	}
	if (!bIsGuarding || bGuardBroken)
	{
		return 1.0f;
	}

	LastHitDirection = ResolveHitDirection(DamageInstigator);
	if (bParryWindowOpen && LastHitDirection == EGPDarkKnightHitDirection::Front)
	{
		SetParryWindowInternal(false);
		if (IsValid(BossOwner))
		{
			// Counter remains a tag-activated GAS ability even though the hit policy detects the successful parry.
			BossOwner->RequestCounterAttackAbility(DamageInstigator);
		}
		return 0.0f;
	}

	float GaugeDamage = FrontGuardGaugeDamage;
	float DamageMultiplier = FrontDamageMultiplier;
	if (LastHitDirection == EGPDarkKnightHitDirection::Side)
	{
		GaugeDamage = SideGuardGaugeDamage;
		DamageMultiplier = SideDamageMultiplier;
	}
	else if (LastHitDirection == EGPDarkKnightHitDirection::Back)
	{
		GaugeDamage = BackGuardGaugeDamage;
		DamageMultiplier = BackDamageMultiplier;
	}
	if (bHeavyAttack)
	{
		GaugeDamage = FMath::Max(GaugeDamage, HeavyGuardGaugeDamage);
	}

	SetGuardGaugeInternal(GuardGauge - GaugeDamage);
	if (GuardGauge <= KINDA_SMALL_NUMBER && IsValid(BossOwner))
	{
		bGuardBroken = true;
		ApplyStateTags();
		BossOwner->RequestEnterGroggyAbility();
	}
	return DamageMultiplier;
}

void UGP_DarkArmorKnightStateComponent::SetCombatPhase(int32 NewPhase)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	const float NewMaxGauge = NewPhase >= 3 ? FinalPhaseMaxGuardGauge : PhaseOneMaxGuardGauge;
	const float PreviousMaxGauge = FMath::Max(1.0f, MaxGuardGauge);
	const float CurrentRatio = FMath::Clamp(GuardGauge / PreviousMaxGauge, 0.0f, 1.0f);
	MaxGuardGauge = FMath::Max(1.0f, NewMaxGauge);
	GuardGauge = bIsGroggy ? 0.0f : MaxGuardGauge * CurrentRatio;
	OnGuardGaugeChanged.Broadcast(GuardGauge, MaxGuardGauge);
}

FName UGP_DarkArmorKnightStateComponent::GetLastHitDirectionName() const
{
	switch (LastHitDirection)
	{
	case EGPDarkKnightHitDirection::Side: return TEXT("Side");
	case EGPDarkKnightHitDirection::Back: return TEXT("Back");
	default: return TEXT("Front");
	}
}

EGPDarkKnightHitDirection UGP_DarkArmorKnightStateComponent::ResolveHitDirection(const AActor* DamageInstigator) const
{
	const AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor) || !IsValid(DamageInstigator))
	{
		return EGPDarkKnightHitDirection::Front;
	}

	const FVector ToInstigator = (DamageInstigator->GetActorLocation() - OwnerActor->GetActorLocation()).GetSafeNormal2D();
	const float ForwardDot = FVector::DotProduct(OwnerActor->GetActorForwardVector().GetSafeNormal2D(), ToInstigator);
	if (ForwardDot >= 0.5f)
	{
		return EGPDarkKnightHitDirection::Front;
	}
	return ForwardDot <= -0.5f ? EGPDarkKnightHitDirection::Back : EGPDarkKnightHitDirection::Side;
}

void UGP_DarkArmorKnightStateComponent::SetGuardGaugeInternal(float NewGauge)
{
	GuardGauge = FMath::Clamp(NewGauge, 0.0f, MaxGuardGauge);
	OnGuardGaugeChanged.Broadcast(GuardGauge, MaxGuardGauge);
}

void UGP_DarkArmorKnightStateComponent::SetGuardingInternal(bool bNewGuarding)
{
	if (bIsGuarding == bNewGuarding)
	{
		return;
	}
	bIsGuarding = bNewGuarding;
	if (GetOwnerRole() == ROLE_Authority && IsValid(BossOwner) && IsValid(BossOwner->GetCharacterMovement()))
	{
		UCharacterMovementComponent* Movement = BossOwner->GetCharacterMovement();
		if (UnguardedMaxWalkSpeed <= KINDA_SMALL_NUMBER)
		{
			UnguardedMaxWalkSpeed = Movement->MaxWalkSpeed;
		}
		// Guarding changes movement policy without leaking that state into the shared behavior tree.
		Movement->MaxWalkSpeed = bIsGuarding
			? UnguardedMaxWalkSpeed * FMath::Clamp(GuardMovementSpeedMultiplier, 0.0f, 1.0f)
			: UnguardedMaxWalkSpeed;
	}
	ApplyStateTags();
	OnGuardingChanged.Broadcast(bIsGuarding);
}

void UGP_DarkArmorKnightStateComponent::SetParryWindowInternal(bool bNewOpen)
{
	if (bParryWindowOpen == bNewOpen)
	{
		return;
	}
	bParryWindowOpen = bNewOpen;
	ApplyStateTags();
	OnParryWindowChanged.Broadcast(bParryWindowOpen);
}

void UGP_DarkArmorKnightStateComponent::SetGroggyInternal(bool bNewGroggy)
{
	if (bIsGroggy == bNewGroggy)
	{
		return;
	}
	bIsGroggy = bNewGroggy;
	ApplyStateTags();
	OnGroggyChanged.Broadcast(bIsGroggy);
}

void UGP_DarkArmorKnightStateComponent::ApplyStateTags()
{
	UAbilitySystemComponent* ASC = ResolveOwnerASC();
	if (!ASC)
	{
		return;
	}

	auto SetLooseTag = [ASC](const FGameplayTag& Tag, bool bShouldApply, bool& bApplied)
	{
		if (bShouldApply && !bApplied)
		{
			ASC->AddLooseGameplayTag(Tag);
			bApplied = true;
		}
		else if (!bShouldApply && bApplied)
		{
			ASC->RemoveLooseGameplayTag(Tag);
			bApplied = false;
		}
	};

	SetLooseTag(GPTags::State::Status::Enemy::DarkGuarded, bIsGuarding && !bIsGroggy && !bGuardBroken, bAppliedGuardTag);
	SetLooseTag(GPTags::State::Status::Enemy::ParryWindow, bParryWindowOpen && bIsGuarding && !bIsGroggy, bAppliedParryTag);
	SetLooseTag(GPTags::State::Status::Enemy::GuardBroken, bGuardBroken, bAppliedBrokenTag);
	SetLooseTag(GPTags::State::Status::Enemy::Groggy, bIsGroggy, bAppliedGroggyTag);
}

UAbilitySystemComponent* UGP_DarkArmorKnightStateComponent::ResolveOwnerASC() const
{
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
}

void UGP_DarkArmorKnightStateComponent::OnRep_GuardGauge()
{
	OnGuardGaugeChanged.Broadcast(GuardGauge, MaxGuardGauge);
}

void UGP_DarkArmorKnightStateComponent::OnRep_IsGuarding()
{
	ApplyStateTags();
	OnGuardingChanged.Broadcast(bIsGuarding);
}

void UGP_DarkArmorKnightStateComponent::OnRep_ParryWindowOpen()
{
	ApplyStateTags();
	OnParryWindowChanged.Broadcast(bParryWindowOpen);
}

void UGP_DarkArmorKnightStateComponent::OnRep_IsGroggy()
{
	ApplyStateTags();
	OnGroggyChanged.Broadcast(bIsGroggy);
}
