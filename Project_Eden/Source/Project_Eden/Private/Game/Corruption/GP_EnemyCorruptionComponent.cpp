#include "Game/Corruption/GP_EnemyCorruptionComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/Effects/GP_EnemyCorruptionGameplayEffect.h"
#include "Game/Corruption/GP_WorldCorruptionComponent.h"
#include "Game/GP_GameState.h"
#include "GameplayTags/GP_Tags.h"

UGP_EnemyCorruptionComponent::UGP_EnemyCorruptionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGP_EnemyCorruptionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindFromWorldCorruption();
	Super::EndPlay(EndPlayReason);
}

void UGP_EnemyCorruptionComponent::InitializeFromOwner()
{
	const AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority())
	{
		return;
	}

	BindToWorldCorruption();
	RefreshCorruptionEffect();
}

void UGP_EnemyCorruptionComponent::SetCorruptionRegionId(int32 InRegionId)
{
	const AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority() || CorruptionRegionId == InRegionId)
	{
		return;
	}

	CorruptionRegionId = InRegionId;
	RefreshCorruptionEffect();
}

void UGP_EnemyCorruptionComponent::HandleOwnerDeath(bool bWasBoss)
{
	AActor* OwnerActor = GetOwner();
	if (!bWasBoss || bBossReductionApplied || !IsValid(OwnerActor) || !OwnerActor->HasAuthority())
	{
		return;
	}

	if (UGP_WorldCorruptionComponent* Corruption = ResolveWorldCorruption())
	{
		const int32 RegionId = ResolveEffectiveRegionId();
		if (RegionId != INDEX_NONE && Corruption->GetRegionCount() > 0)
		{
			// A boss cleanses its authored region; this also lowers the derived world average.
			Corruption->ReduceRegionCorruption(RegionId, BossDefeatCorruptionReduction);
		}
		else
		{
			// Maps without regional data still receive the expected global cleanse.
			Corruption->AddWorldCorruption(-BossDefeatCorruptionReduction);
		}
		bBossReductionApplied = true;
	}
}

void UGP_EnemyCorruptionComponent::HandleWorldCorruptionChanged(float Corruption, float NormalizedCorruption)
{
	if (BoundWorldCorruption.IsValid() && BoundWorldCorruption->GetRegionCount() == 0)
	{
		RefreshCorruptionEffect();
	}
}

void UGP_EnemyCorruptionComponent::HandleRegionCorruptionChanged(int32 RegionId, float Corruption, float NormalizedCorruption)
{
	if (RegionId == ResolveEffectiveRegionId())
	{
		RefreshCorruptionEffect();
	}
}

void UGP_EnemyCorruptionComponent::BindToWorldCorruption()
{
	UGP_WorldCorruptionComponent* Corruption = ResolveWorldCorruption();
	if (!IsValid(Corruption) || BoundWorldCorruption.Get() == Corruption)
	{
		return;
	}

	UnbindFromWorldCorruption();
	BoundWorldCorruption = Corruption;
	Corruption->OnWorldCorruptionChanged.AddUniqueDynamic(this, &ThisClass::HandleWorldCorruptionChanged);
	Corruption->OnRegionCorruptionChanged.AddUniqueDynamic(this, &ThisClass::HandleRegionCorruptionChanged);
}

void UGP_EnemyCorruptionComponent::UnbindFromWorldCorruption()
{
	if (UGP_WorldCorruptionComponent* Corruption = BoundWorldCorruption.Get())
	{
		Corruption->OnWorldCorruptionChanged.RemoveDynamic(this, &ThisClass::HandleWorldCorruptionChanged);
		Corruption->OnRegionCorruptionChanged.RemoveDynamic(this, &ThisClass::HandleRegionCorruptionChanged);
	}
	BoundWorldCorruption.Reset();
}

void UGP_EnemyCorruptionComponent::RefreshCorruptionEffect()
{
	UAbilitySystemComponent* ASC = ResolveOwnerASC();
	UGP_WorldCorruptionComponent* Corruption = ResolveWorldCorruption();
	if (!IsValid(ASC) || !IsValid(Corruption))
	{
		return;
	}

	if (ActiveCorruptionEffectHandle.IsValid())
	{
		// Replacing only our own handle keeps unrelated GAS buffs and initialization effects intact.
		ASC->RemoveActiveGameplayEffect(ActiveCorruptionEffectHandle);
		ActiveCorruptionEffectHandle.Invalidate();
	}

	AppliedCorruptionNormalized = Corruption->GetRegionCorruptionNormalized(ResolveEffectiveRegionId());
	if (AppliedCorruptionNormalized <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);
	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(
		UGP_EnemyCorruptionGameplayEffect::StaticClass(),
		1.0f,
		EffectContext);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(
		GPTags::Corruption::Data::DamageIncreaseRate,
		AppliedCorruptionNormalized * MaximumDamageIncreaseRate);
	SpecHandle.Data->SetSetByCallerMagnitude(
		GPTags::Corruption::Data::ArmorBonus,
		AppliedCorruptionNormalized * MaximumArmorBonus);
	ActiveCorruptionEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

int32 UGP_EnemyCorruptionComponent::ResolveEffectiveRegionId() const
{
	if (CorruptionRegionId != INDEX_NONE)
	{
		return CorruptionRegionId;
	}

	const UWorld* World = GetWorld();
	const AGP_GameState* GameState = World ? World->GetGameState<AGP_GameState>() : nullptr;
	return IsValid(GameState) ? GameState->GetCurrentZoneIndex() : INDEX_NONE;
}

UGP_WorldCorruptionComponent* UGP_EnemyCorruptionComponent::ResolveWorldCorruption() const
{
	const UWorld* World = GetWorld();
	AGP_GameState* GameState = World ? World->GetGameState<AGP_GameState>() : nullptr;
	return IsValid(GameState) ? GameState->GetWorldCorruptionComponent() : nullptr;
}

UAbilitySystemComponent* UGP_EnemyCorruptionComponent::ResolveOwnerASC() const
{
	const IAbilitySystemInterface* AbilitySystemOwner = Cast<IAbilitySystemInterface>(GetOwner());
	return AbilitySystemOwner ? AbilitySystemOwner->GetAbilitySystemComponent() : nullptr;
}
