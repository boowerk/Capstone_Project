#include "Game/Corruption/GP_WorldCorruptionComponent.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

UGP_WorldCorruptionComponent::UGP_WorldCorruptionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UGP_WorldCorruptionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopPassiveIncrease();
	Super::EndPlay(EndPlayReason);
}

void UGP_WorldCorruptionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGP_WorldCorruptionComponent, WorldCorruption);
	DOREPLIFETIME(UGP_WorldCorruptionComponent, RegionCorruptions);
	DOREPLIFETIME(UGP_WorldCorruptionComponent, MaximumCorruption);
}

float UGP_WorldCorruptionComponent::GetWorldCorruptionNormalized() const
{
	return MaximumCorruption > KINDA_SMALL_NUMBER
		? FMath::Clamp(WorldCorruption / MaximumCorruption, 0.0f, 1.0f)
		: 0.0f;
}

float UGP_WorldCorruptionComponent::GetRegionCorruption(int32 RegionId) const
{
	// Maps without authored regions still expose their global corruption as a useful fallback.
	return RegionCorruptions.IsValidIndex(RegionId) ? RegionCorruptions[RegionId] : WorldCorruption;
}

float UGP_WorldCorruptionComponent::GetRegionCorruptionNormalized(int32 RegionId) const
{
	return MaximumCorruption > KINDA_SMALL_NUMBER
		? FMath::Clamp(GetRegionCorruption(RegionId) / MaximumCorruption, 0.0f, 1.0f)
		: 0.0f;
}

void UGP_WorldCorruptionComponent::InitializeCorruption(
	int32 RegionCount,
	float InitialCorruption,
	float InMaximumCorruption,
	float InPassiveIncreasePerMinute,
	float InPassiveTickInterval,
	bool bEnablePassiveIncrease)
{
	if (!HasServerAuthority())
	{
		return;
	}

	MaximumCorruption = FMath::Max(1.0f, InMaximumCorruption);
	PassiveIncreasePerMinute = FMath::Max(0.0f, InPassiveIncreasePerMinute);
	PassiveTickInterval = FMath::Max(0.1f, InPassiveTickInterval);
	const float ClampedInitialCorruption = ClampCorruption(InitialCorruption);

	RegionCorruptions.Init(ClampedInitialCorruption, FMath::Max(0, RegionCount));
	LocalRegionCorruptionsShadow = RegionCorruptions;
	WorldCorruption = ClampedInitialCorruption;

	BroadcastWorldCorruption();
	for (int32 RegionId = 0; RegionId < RegionCorruptions.Num(); ++RegionId)
	{
		BroadcastRegionCorruption(RegionId);
	}

	StopPassiveIncrease();
	if (bEnablePassiveIncrease && PassiveIncreasePerMinute > KINDA_SMALL_NUMBER)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				PassiveIncreaseTimerHandle,
				this,
				&ThisClass::HandlePassiveIncreaseTick,
				PassiveTickInterval,
				true);
		}
	}

	if (AActor* OwnerActor = GetOwner())
	{
		OwnerActor->ForceNetUpdate();
	}
}

void UGP_WorldCorruptionComponent::SetWorldCorruption(float NewCorruption)
{
	if (!HasServerAuthority())
	{
		return;
	}

	const float ClampedCorruption = ClampCorruption(NewCorruption);
	if (RegionCorruptions.IsEmpty())
	{
		if (!FMath::IsNearlyEqual(WorldCorruption, ClampedCorruption))
		{
			WorldCorruption = ClampedCorruption;
			BroadcastWorldCorruption();
		}
		return;
	}

	for (int32 RegionId = 0; RegionId < RegionCorruptions.Num(); ++RegionId)
	{
		SetRegionCorruption(RegionId, ClampedCorruption);
	}
}

void UGP_WorldCorruptionComponent::AddWorldCorruption(float DeltaCorruption)
{
	if (!HasServerAuthority() || FMath::IsNearlyZero(DeltaCorruption))
	{
		return;
	}

	if (RegionCorruptions.IsEmpty())
	{
		SetWorldCorruption(WorldCorruption + DeltaCorruption);
		return;
	}

	// A world-wide change advances every region equally; their average remains the global value.
	for (int32 RegionId = 0; RegionId < RegionCorruptions.Num(); ++RegionId)
	{
		RegionCorruptions[RegionId] = ClampCorruption(RegionCorruptions[RegionId] + DeltaCorruption);
		BroadcastRegionCorruption(RegionId);
	}
	LocalRegionCorruptionsShadow = RegionCorruptions;
	RecalculateWorldCorruption();
}

void UGP_WorldCorruptionComponent::SetRegionCorruption(int32 RegionId, float NewCorruption)
{
	if (!HasServerAuthority() || !RegionCorruptions.IsValidIndex(RegionId))
	{
		return;
	}

	const float ClampedCorruption = ClampCorruption(NewCorruption);
	if (FMath::IsNearlyEqual(RegionCorruptions[RegionId], ClampedCorruption))
	{
		return;
	}

	RegionCorruptions[RegionId] = ClampedCorruption;
	LocalRegionCorruptionsShadow = RegionCorruptions;
	BroadcastRegionCorruption(RegionId);
	RecalculateWorldCorruption();
}

void UGP_WorldCorruptionComponent::AddRegionCorruption(int32 RegionId, float DeltaCorruption)
{
	if (RegionCorruptions.IsValidIndex(RegionId))
	{
		SetRegionCorruption(RegionId, RegionCorruptions[RegionId] + DeltaCorruption);
	}
}

void UGP_WorldCorruptionComponent::ReduceRegionCorruption(int32 RegionId, float ReductionAmount)
{
	AddRegionCorruption(RegionId, -FMath::Abs(ReductionAmount));
}

void UGP_WorldCorruptionComponent::OnRep_WorldCorruption()
{
	BroadcastWorldCorruption();
}

void UGP_WorldCorruptionComponent::OnRep_RegionCorruptions()
{
	if (LocalRegionCorruptionsShadow.Num() != RegionCorruptions.Num())
	{
		LocalRegionCorruptionsShadow = RegionCorruptions;
		for (int32 RegionId = 0; RegionId < RegionCorruptions.Num(); ++RegionId)
		{
			BroadcastRegionCorruption(RegionId);
		}
		return;
	}

	for (int32 RegionId = 0; RegionId < RegionCorruptions.Num(); ++RegionId)
	{
		if (!FMath::IsNearlyEqual(LocalRegionCorruptionsShadow[RegionId], RegionCorruptions[RegionId]))
		{
			BroadcastRegionCorruption(RegionId);
		}
	}
	LocalRegionCorruptionsShadow = RegionCorruptions;
}

void UGP_WorldCorruptionComponent::OnRep_MaximumCorruption()
{
	// The raw value may be unchanged while its normalized presentation changed with the replicated ceiling.
	BroadcastWorldCorruption();
	for (int32 RegionId = 0; RegionId < RegionCorruptions.Num(); ++RegionId)
	{
		BroadcastRegionCorruption(RegionId);
	}
}

void UGP_WorldCorruptionComponent::HandlePassiveIncreaseTick()
{
	const float TickIncrease = PassiveIncreasePerMinute * (PassiveTickInterval / 60.0f);
	AddWorldCorruption(TickIncrease);
}

void UGP_WorldCorruptionComponent::RecalculateWorldCorruption()
{
	float NewWorldCorruption = WorldCorruption;
	if (!RegionCorruptions.IsEmpty())
	{
		double TotalCorruption = 0.0;
		for (const float RegionCorruption : RegionCorruptions)
		{
			TotalCorruption += RegionCorruption;
		}
		NewWorldCorruption = ClampCorruption(static_cast<float>(TotalCorruption / RegionCorruptions.Num()));
	}

	if (!FMath::IsNearlyEqual(WorldCorruption, NewWorldCorruption))
	{
		WorldCorruption = NewWorldCorruption;
		BroadcastWorldCorruption();
	}

	if (AActor* OwnerActor = GetOwner())
	{
		OwnerActor->ForceNetUpdate();
	}
}

void UGP_WorldCorruptionComponent::BroadcastWorldCorruption()
{
	OnWorldCorruptionChanged.Broadcast(WorldCorruption, GetWorldCorruptionNormalized());
}

void UGP_WorldCorruptionComponent::BroadcastRegionCorruption(int32 RegionId)
{
	OnRegionCorruptionChanged.Broadcast(
		RegionId,
		GetRegionCorruption(RegionId),
		GetRegionCorruptionNormalized(RegionId));
}

void UGP_WorldCorruptionComponent::StopPassiveIncrease()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PassiveIncreaseTimerHandle);
	}
}

bool UGP_WorldCorruptionComponent::HasServerAuthority() const
{
	// Owner-less components are allowed for deterministic automation tests.
	const AActor* OwnerActor = GetOwner();
	return OwnerActor == nullptr || OwnerActor->HasAuthority();
}

float UGP_WorldCorruptionComponent::ClampCorruption(float Value) const
{
	return FMath::Clamp(Value, 0.0f, MaximumCorruption);
}
