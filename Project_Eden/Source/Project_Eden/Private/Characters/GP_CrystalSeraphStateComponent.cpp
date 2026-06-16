#include "Characters/GP_CrystalSeraphStateComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayTags/GP_Tags.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

UGP_CrystalSeraphStateComponent::UGP_CrystalSeraphStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UGP_CrystalSeraphStateComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!MainBossActor)
	{
		MainBossActor = GetOwner();
	}

	// Apply the default guarded tag from C++ so native and Blueprint child bosses share the same damage rules.
	if (GetOwnerRole() == ROLE_Authority)
	{
		ApplyStateTags();
	}
}

void UGP_CrystalSeraphStateComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WingCoreExposureTimerHandle);
	}

	SetWingCoreExposedInternal(false);
	SetGuardedInternal(false);
	SetGroggyInternal(false);

	Super::EndPlay(EndPlayReason);
}

void UGP_CrystalSeraphStateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGP_CrystalSeraphStateComponent, WingCoreBreakCount);
	DOREPLIFETIME(UGP_CrystalSeraphStateComponent, bIsGroggy);
	DOREPLIFETIME(UGP_CrystalSeraphStateComponent, bIsGuarded);
	DOREPLIFETIME(UGP_CrystalSeraphStateComponent, bWingCoreExposed);
	DOREPLIFETIME(UGP_CrystalSeraphStateComponent, WingCoreHealth);
	DOREPLIFETIME(UGP_CrystalSeraphStateComponent, WingCoreMaxHealth);
	DOREPLIFETIME(UGP_CrystalSeraphStateComponent, MainBossActor);
	DOREPLIFETIME(UGP_CrystalSeraphStateComponent, CrystalPrismActor);
	DOREPLIFETIME(UGP_CrystalSeraphStateComponent, WingCoreActor);
}

void UGP_CrystalSeraphStateComponent::InitializeCrystalSeraphState(AActor* InMainBossActor)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	MainBossActor = InMainBossActor ? InMainBossActor : GetOwner();
	CrystalPrismActor = nullptr;
	WingCoreActor = nullptr;
	ResetWingCoreBreakCount();
	SetWingCoreExposedInternal(false);
	SetGroggyInternal(false);
	SetGuardedInternal(true);
}

void UGP_CrystalSeraphStateComponent::RegisterCrystalPrismActor(AActor* InCrystalPrismActor)
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		CrystalPrismActor = InCrystalPrismActor;
	}
}

void UGP_CrystalSeraphStateComponent::RegisterWingCoreActor(AActor* InWingCoreActor)
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		WingCoreActor = InWingCoreActor;
	}
}

void UGP_CrystalSeraphStateComponent::BeginWingCoreExposure(float BossMaxHealth, float ExposureDurationOverride)
{
	if (GetOwnerRole() != ROLE_Authority || bIsGroggy)
	{
		return;
	}

	ResetWingCoreHealth(BossMaxHealth);
	SetGuardedInternal(false);
	SetWingCoreExposedInternal(true);

	const float ExposureDuration = ExposureDurationOverride >= 0.0f
		? ExposureDurationOverride
		: DefaultWingCoreExposureDuration;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WingCoreExposureTimerHandle);
		if (ExposureDuration > 0.0f)
		{
			World->GetTimerManager().SetTimer(
				WingCoreExposureTimerHandle,
				this,
				&ThisClass::EndWingCoreExposure,
				ExposureDuration,
				false);
		}
	}
}

void UGP_CrystalSeraphStateComponent::EndWingCoreExposure()
{
	if (GetOwnerRole() != ROLE_Authority || bIsGroggy)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WingCoreExposureTimerHandle);
	}

	SetWingCoreExposedInternal(false);
	SetGuardedInternal(true);
	WingCoreHealth = 0.0f;
}

bool UGP_CrystalSeraphStateComponent::RecordWingCoreDamage(float DamageAmount, float BossMaxHealth)
{
	if (GetOwnerRole() != ROLE_Authority || !bWingCoreExposed || bIsGroggy || DamageAmount <= 0.0f)
	{
		return false;
	}

	if (WingCoreMaxHealth <= KINDA_SMALL_NUMBER)
	{
		ResetWingCoreHealth(BossMaxHealth);
	}

	WingCoreHealth = FMath::Max(0.0f, WingCoreHealth - DamageAmount);
	if (WingCoreHealth > KINDA_SMALL_NUMBER)
	{
		return false;
	}

	SetWingCoreBreakCount(WingCoreBreakCount + 1);
	SetWingCoreExposedInternal(false);

	if (WingCoreBreakCount >= GetWingCoreBreakTarget())
	{
		EnterGroggy();
	}
	else
	{
		SetGuardedInternal(true);
	}

	return true;
}

void UGP_CrystalSeraphStateComponent::SetWingCoreBreakCount(int32 NewCount)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	const int32 ClampedCount = FMath::Clamp(NewCount, 0, GetWingCoreBreakTarget());
	if (WingCoreBreakCount == ClampedCount)
	{
		return;
	}

	WingCoreBreakCount = ClampedCount;
	OnWingCoreStageChanged.Broadcast(WingCoreBreakCount, GetWingCoreBreakTarget());
}

void UGP_CrystalSeraphStateComponent::ResetWingCoreBreakCount()
{
	SetWingCoreBreakCount(0);
}

void UGP_CrystalSeraphStateComponent::EnterGroggy()
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WingCoreExposureTimerHandle);
	}

	SetWingCoreBreakCount(GetWingCoreBreakTarget());
	SetWingCoreExposedInternal(false);
	SetGuardedInternal(false);
	SetGroggyInternal(true);
}

void UGP_CrystalSeraphStateComponent::RecoverFromGroggy()
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	ResetWingCoreBreakCount();
	WingCoreHealth = 0.0f;
	SetGroggyInternal(false);
	SetWingCoreExposedInternal(false);
	SetGuardedInternal(true);
}

void UGP_CrystalSeraphStateComponent::OnRep_WingCoreBreakCount()
{
	OnWingCoreStageChanged.Broadcast(WingCoreBreakCount, GetWingCoreBreakTarget());
}

void UGP_CrystalSeraphStateComponent::OnRep_IsGroggy()
{
	ApplyStateTags();
	OnGroggyChanged.Broadcast(bIsGroggy);
}

void UGP_CrystalSeraphStateComponent::OnRep_IsGuarded()
{
	ApplyStateTags();
}

void UGP_CrystalSeraphStateComponent::OnRep_WingCoreExposed()
{
	ApplyStateTags();
	OnWingCoreExposedChanged.Broadcast(bWingCoreExposed);
}

void UGP_CrystalSeraphStateComponent::SetGroggyInternal(bool bNewGroggy)
{
	if (bIsGroggy == bNewGroggy)
	{
		return;
	}

	bIsGroggy = bNewGroggy;
	ApplyStateTags();
	OnGroggyChanged.Broadcast(bIsGroggy);
}

void UGP_CrystalSeraphStateComponent::SetGuardedInternal(bool bNewGuarded)
{
	if (bIsGuarded == bNewGuarded)
	{
		return;
	}

	bIsGuarded = bNewGuarded;
	ApplyStateTags();
}

void UGP_CrystalSeraphStateComponent::SetWingCoreExposedInternal(bool bNewExposed)
{
	if (bWingCoreExposed == bNewExposed)
	{
		return;
	}

	bWingCoreExposed = bNewExposed;
	ApplyStateTags();
	OnWingCoreExposedChanged.Broadcast(bWingCoreExposed);
}

void UGP_CrystalSeraphStateComponent::ResetWingCoreHealth(float BossMaxHealth)
{
	WingCoreMaxHealth = FMath::Max(1.0f, BossMaxHealth * FMath::Clamp(WingCoreHealthFraction, 0.0f, 1.0f));
	WingCoreHealth = WingCoreMaxHealth;
}

void UGP_CrystalSeraphStateComponent::ApplyStateTags()
{
	UAbilitySystemComponent* ASC = ResolveOwnerASC();
	if (!ASC)
	{
		return;
	}

	const bool bShouldApplyGuarded = bIsGuarded && !bIsGroggy && !bWingCoreExposed;
	if (bShouldApplyGuarded && !bAppliedCrystalGuardedTag)
	{
		ASC->AddLooseGameplayTag(GPTags::State::Status::Enemy::CrystalGuarded);
		bAppliedCrystalGuardedTag = true;
	}
	else if (!bShouldApplyGuarded && bAppliedCrystalGuardedTag)
	{
		ASC->RemoveLooseGameplayTag(GPTags::State::Status::Enemy::CrystalGuarded);
		bAppliedCrystalGuardedTag = false;
	}

	const bool bShouldApplyCoreExposed = bWingCoreExposed && !bIsGroggy;
	if (bShouldApplyCoreExposed && !bAppliedWingCoreExposedTag)
	{
		ASC->AddLooseGameplayTag(GPTags::State::Status::Enemy::WingCoreExposed);
		bAppliedWingCoreExposedTag = true;
	}
	else if (!bShouldApplyCoreExposed && bAppliedWingCoreExposedTag)
	{
		ASC->RemoveLooseGameplayTag(GPTags::State::Status::Enemy::WingCoreExposed);
		bAppliedWingCoreExposedTag = false;
	}

	if (bIsGroggy && !bAppliedGroggyTag)
	{
		ASC->AddLooseGameplayTag(GPTags::State::Status::Enemy::Groggy);
		bAppliedGroggyTag = true;
	}
	else if (!bIsGroggy && bAppliedGroggyTag)
	{
		ASC->RemoveLooseGameplayTag(GPTags::State::Status::Enemy::Groggy);
		bAppliedGroggyTag = false;
	}
}

UAbilitySystemComponent* UGP_CrystalSeraphStateComponent::ResolveOwnerASC() const
{
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
}
