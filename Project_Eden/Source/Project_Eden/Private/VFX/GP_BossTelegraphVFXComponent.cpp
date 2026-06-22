#include "VFX/GP_BossTelegraphVFXComponent.h"

#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

UGP_BossTelegraphVFXComponent::UGP_BossTelegraphVFXComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
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
	if (!GetAsset() && IsValid(DefaultTelegraphSystem))
	{
		SetAsset(DefaultTelegraphSystem);
	}
	ApplyPresentationSettings();
	Activate(true);
}

void UGP_BossTelegraphVFXComponent::StopTelegraph()
{
	DeactivateImmediate();
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
