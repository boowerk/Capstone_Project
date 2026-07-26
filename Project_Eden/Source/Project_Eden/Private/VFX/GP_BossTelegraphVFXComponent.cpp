#include "VFX/GP_BossTelegraphVFXComponent.h"

#include "GameFramework/Actor.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

UGP_BossTelegraphVFXComponent::UGP_BossTelegraphVFXComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	// Native boss-owned instances use a designer toggle; replication lets the server start the same cue for all clients.
	SetIsReplicatedByDefault(true);
	SetAutoActivate(true);

	// Supply the requested lightning as a reusable default while keeping System Asset editable per Blueprint instance.
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> DefaultTelegraphFinder(
		TEXT("/Game/Niagara/Vefects/Easy_Impact_Frames/VFX/Extras/Particles/NS_Extra_Lightning_Example_VFX.NS_Extra_Lightning_Example_VFX"));
	if (DefaultTelegraphFinder.Succeeded())
	{
		DefaultTelegraphSystem = DefaultTelegraphFinder.Object;
	}

	ApplyPresentationSettings();
}

void UGP_BossTelegraphVFXComponent::PlayTelegraph()
{
	PlayTelegraphLocal();
}

float UGP_BossTelegraphVFXComponent::PlayEnabledTelegraph()
{
	if (!bTelegraphVFXEnabled)
	{
		return 0.0f;
	}

	AActor* OwnerActor = GetOwner();
	if (IsValid(OwnerActor) && OwnerActor->HasAuthority())
	{
		MulticastPlayTelegraph();
	}
	else
	{
		// Editor previews and non-networked component instances still need an immediate local path.
		PlayTelegraphLocal();
	}

	return FMath::Max(0.0f, TelegraphDuration);
}

bool UGP_BossTelegraphVFXComponent::IsPatternTelegraphEnabled(
	FGameplayTag PatternTag,
	const TMap<FGameplayTag, bool>& PatternToggles) const
{
	const bool* bPatternEnabled = PatternToggles.Find(PatternTag);
	return bTelegraphVFXEnabled && bPatternEnabled != nullptr && *bPatternEnabled;
}

float UGP_BossTelegraphVFXComponent::PlayPatternTelegraph(
	FGameplayTag PatternTag,
	const TMap<FGameplayTag, bool>& PatternToggles)
{
	// The master switch alone never enables every attack; each pattern must be opted in from the owning boss Blueprint.
	return IsPatternTelegraphEnabled(PatternTag, PatternToggles)
		? PlayEnabledTelegraph()
		: 0.0f;
}

void UGP_BossTelegraphVFXComponent::MulticastPlayTelegraph_Implementation()
{
	PlayTelegraphLocal();
}

void UGP_BossTelegraphVFXComponent::PlayTelegraphLocal()
{
	if (!GetAsset() && IsValid(DefaultTelegraphSystem))
	{
		SetAsset(DefaultTelegraphSystem);
	}
	ApplyPresentationSettings();
	bExplicitActivationInProgress = true;
	Activate(true);
	bExplicitActivationInProgress = false;
}

void UGP_BossTelegraphVFXComponent::StopTelegraph()
{
	DeactivateImmediate();
}

void UGP_BossTelegraphVFXComponent::SetDefaultTelegraphSystem(UNiagaraSystem* InTelegraphSystem)
{
	DefaultTelegraphSystem = InTelegraphSystem;
	if (IsRegistered())
	{
		SetAsset(DefaultTelegraphSystem);
	}
}

void UGP_BossTelegraphVFXComponent::OnRegister()
{
	// Delay SetAsset until registration; calling it from a component CDO constructor is unsafe in Niagara.
	if (!GetAsset() && IsValid(DefaultTelegraphSystem))
	{
		SetAsset(DefaultTelegraphSystem);
	}
	Super::OnRegister();
	ApplyPresentationSettings();
}

void UGP_BossTelegraphVFXComponent::Activate(bool bReset)
{
	if (!bTelegraphVFXEnabled && !bExplicitActivationInProgress)
	{
		// Telegraph VFX On/Off remains authoritative when an older Blueprint serialized Auto Activate as true.
		DeactivateImmediate();
		return;
	}

	Super::Activate(bReset);
}

#if WITH_EDITOR
void UGP_BossTelegraphVFXComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	// Refresh the preview immediately when a designer changes component scale in a Blueprint editor.
	ApplyPresentationSettings();
}
#endif

void UGP_BossTelegraphVFXComponent::ApplyPresentationSettings()
{
	SetRelativeScale3D(FVector(FMath::Max(0.01f, UniformVisualScale)));
}
