#include "VFX/GP_VisualCueComponent.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "VFX/GP_VisualCueResolver.h"

UGP_VisualCueComponent::UGP_VisualCueComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGP_VisualCueComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DeactivateAllPersistentCues();
	Super::EndPlay(EndPlayReason);
}

void UGP_VisualCueComponent::AddNiagaraCue(const FGameplayTag& CueTag, UNiagaraSystem* NiagaraSystem, const FGameplayTag& ElementTag)
{
	if (!CueTag.IsValid() || !IsValid(NiagaraSystem))
	{
		return;
	}

	for (FGP_SkillVisualCueEntry& Entry : VisualCues)
	{
		if (Entry.VisualType == EGP_SkillVisualType::Niagara
			&& Entry.CueTag.MatchesTagExact(CueTag)
			&& Entry.ElementTag == ElementTag)
		{
			Entry.NiagaraSystem = NiagaraSystem;
			return;
		}
	}

	FGP_SkillVisualCueEntry& NewEntry = VisualCues.AddDefaulted_GetRef();
	NewEntry.CueTag = CueTag;
	NewEntry.ElementTag = ElementTag;
	NewEntry.VisualType = EGP_SkillVisualType::Niagara;
	NewEntry.NiagaraSystem = NiagaraSystem;
}

UNiagaraSystem* UGP_VisualCueComponent::ResolveNiagara(FGameplayTag CueTag, FGameplayTag ElementTag) const
{
	return GPVisualCueResolver::ResolveNiagara(VisualCues, ElementTag, CueTag);
}

UNiagaraComponent* UGP_VisualCueComponent::ActivatePersistentCue(
	const FGameplayTag& CueTag,
	USceneComponent* AttachComponent,
	const FVector& RelativeLocation,
	const FRotator& RelativeRotation,
	const FVector& RelativeScale)
{
	if (TObjectPtr<UNiagaraComponent>* ExistingComponent = ActiveCueComponents.Find(CueTag))
	{
		if (IsValid(*ExistingComponent))
		{
			return ExistingComponent->Get();
		}
		ActiveCueComponents.Remove(CueTag);
	}

	UNiagaraSystem* NiagaraSystem = ResolveNiagara(CueTag);
	if (!IsValid(NiagaraSystem) || !IsValid(AttachComponent))
	{
		return nullptr;
	}

	UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
		NiagaraSystem,
		AttachComponent,
		NAME_None,
		RelativeLocation,
		RelativeRotation,
		EAttachLocation::KeepRelativeOffset,
		false,
		false,
		ENCPoolMethod::None,
		true);
	if (IsValid(NiagaraComponent))
	{
		NiagaraComponent->SetRelativeScale3D(RelativeScale);
		ApplyNiagaraTintOverride(NiagaraComponent);
		NiagaraComponent->Activate(true);
		ActiveCueComponents.Add(CueTag, NiagaraComponent);
	}
	return NiagaraComponent;
}

void UGP_VisualCueComponent::DeactivatePersistentCue(const FGameplayTag& CueTag)
{
	TObjectPtr<UNiagaraComponent> NiagaraComponent;
	if (!ActiveCueComponents.RemoveAndCopyValue(CueTag, NiagaraComponent) || !IsValid(NiagaraComponent))
	{
		return;
	}

	NiagaraComponent->DeactivateImmediate();
	NiagaraComponent->DestroyComponent();
}

void UGP_VisualCueComponent::DeactivateAllPersistentCues()
{
	for (TPair<FGameplayTag, TObjectPtr<UNiagaraComponent>>& Pair : ActiveCueComponents)
	{
		if (IsValid(Pair.Value))
		{
			Pair.Value->DeactivateImmediate();
			Pair.Value->DestroyComponent();
		}
	}
	ActiveCueComponents.Reset();
}

UNiagaraComponent* UGP_VisualCueComponent::PlayOneShotAttached(
	const FGameplayTag& CueTag,
	USceneComponent* AttachComponent,
	const FVector& RelativeLocation,
	const FRotator& RelativeRotation,
	const FVector& RelativeScale) const
{
	UNiagaraSystem* NiagaraSystem = ResolveNiagara(CueTag);
	if (!IsValid(NiagaraSystem) || !IsValid(AttachComponent))
	{
		return nullptr;
	}

	UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
		NiagaraSystem,
		AttachComponent,
		NAME_None,
		RelativeLocation,
		RelativeRotation,
		EAttachLocation::KeepRelativeOffset,
		true,
		false,
		ENCPoolMethod::AutoRelease,
		true);
	if (IsValid(NiagaraComponent))
	{
		NiagaraComponent->SetRelativeScale3D(RelativeScale);
		ApplyNiagaraTintOverride(NiagaraComponent);
		NiagaraComponent->Activate(true);
	}
	return NiagaraComponent;
}

UNiagaraComponent* UGP_VisualCueComponent::PlayOneShotAtLocation(
	const FGameplayTag& CueTag,
	const FVector& WorldLocation,
	const FRotator& WorldRotation,
	const FVector& WorldScale) const
{
	UNiagaraSystem* NiagaraSystem = ResolveNiagara(CueTag);
	UNiagaraComponent* NiagaraComponent = IsValid(NiagaraSystem)
		? UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			NiagaraSystem,
			WorldLocation,
			WorldRotation,
			WorldScale,
			true,
			false,
			ENCPoolMethod::AutoRelease)
		: nullptr;
	if (IsValid(NiagaraComponent))
	{
		ApplyNiagaraTintOverride(NiagaraComponent);
		NiagaraComponent->Activate(true);
	}
	return NiagaraComponent;
}

void UGP_VisualCueComponent::SetNiagaraTintOverride(bool bEnabled, const FLinearColor& InTintColor)
{
	bApplyNiagaraTintOverride = bEnabled;
	NiagaraTintOverrideColor = InTintColor;
}

void UGP_VisualCueComponent::ApplyNiagaraTintOverride(UNiagaraComponent* NiagaraComponent) const
{
	if (!bApplyNiagaraTintOverride || !IsValid(NiagaraComponent))
	{
		return;
	}

	static const FName TintParameterNames[] =
	{
		TEXT("User.Color"),
		TEXT("User.Tint"),
		TEXT("User.TintColor"),
		TEXT("User.ParticleColor"),
		TEXT("User.Color_Main"),
		TEXT("User.Color1"),
		TEXT("User.Color2"),
		TEXT("User.Color_Ray"),
		TEXT("User.Color_Smoke"),
		TEXT("User.Color_Sparks1"),
		TEXT("User.Color_Sparks2"),
		TEXT("User.Color_Spiral1"),
		TEXT("User.Color_Trace"),
		TEXT("User.Color_Wave")
	};

	for (const FName TintParameterName : TintParameterNames)
	{
		// Several marketplace Niagara packs use different User color names, so apply the same tint to the common set.
		NiagaraComponent->SetVariableLinearColor(TintParameterName, NiagaraTintOverrideColor);
	}
}
