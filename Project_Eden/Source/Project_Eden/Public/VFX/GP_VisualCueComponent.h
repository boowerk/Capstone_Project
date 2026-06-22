#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_SkillData.h"
#include "Components/ActorComponent.h"
#include "GP_VisualCueComponent.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;

/** Actor-side counterpart to player SkillData visuals: resolve by cue, own persistent effects, and spawn local one-shots. */
UCLASS(ClassGroup = (VFX), meta = (BlueprintSpawnableComponent))
class PROJECT_EDEN_API UGP_VisualCueComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGP_VisualCueComponent();
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void AddNiagaraCue(const FGameplayTag& CueTag, UNiagaraSystem* NiagaraSystem, const FGameplayTag& ElementTag = FGameplayTag());

	UFUNCTION(BlueprintPure, Category = "VFX|Visual Cue")
	UNiagaraSystem* ResolveNiagara(FGameplayTag CueTag, FGameplayTag ElementTag = FGameplayTag()) const;

	UNiagaraComponent* ActivatePersistentCue(
		const FGameplayTag& CueTag,
		USceneComponent* AttachComponent,
		const FVector& RelativeLocation = FVector::ZeroVector,
		const FRotator& RelativeRotation = FRotator::ZeroRotator,
		const FVector& RelativeScale = FVector::OneVector);

	void DeactivatePersistentCue(const FGameplayTag& CueTag);
	void DeactivateAllPersistentCues();

	UNiagaraComponent* PlayOneShotAttached(
		const FGameplayTag& CueTag,
		USceneComponent* AttachComponent,
		const FVector& RelativeLocation = FVector::ZeroVector,
		const FRotator& RelativeRotation = FRotator::ZeroRotator,
		const FVector& RelativeScale = FVector::OneVector) const;

	UNiagaraComponent* PlayOneShotAtLocation(
		const FGameplayTag& CueTag,
		const FVector& WorldLocation,
		const FRotator& WorldRotation = FRotator::ZeroRotator,
		const FVector& WorldScale = FVector::OneVector) const;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX|Visual Cue", meta = (AllowPrivateAccess = "true", TitleProperty = "CueTag"))
	TArray<FGP_SkillVisualCueEntry> VisualCues;

	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<UNiagaraComponent>> ActiveCueComponents;
};
