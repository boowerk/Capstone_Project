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
		true,
		ENCPoolMethod::None,
		true);
	if (IsValid(NiagaraComponent))
	{
		NiagaraComponent->SetRelativeScale3D(RelativeScale);
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
		true,
		ENCPoolMethod::AutoRelease,
		true);
	if (IsValid(NiagaraComponent))
	{
		NiagaraComponent->SetRelativeScale3D(RelativeScale);
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
	return IsValid(NiagaraSystem)
		? UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			NiagaraSystem,
			WorldLocation,
			WorldRotation,
			WorldScale,
			true,
			true,
			ENCPoolMethod::AutoRelease)
		: nullptr;
}
