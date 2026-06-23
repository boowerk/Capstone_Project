#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "GP_BossTargetMarkerVFXComponent.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;
class USceneComponent;

/** Boss-owned helper that marks the player currently selected by AI targeting. */
UCLASS(ClassGroup = (Boss), meta = (BlueprintSpawnableComponent, DisplayName = "Boss Target Marker VFX"))
class PROJECT_EDEN_API UGP_BossTargetMarkerVFXComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGP_BossTargetMarkerVFXComponent();

	UFUNCTION(BlueprintCallable, Category = "Boss|Target Marker")
	void PlayTargetMarker(AActor* TargetActor);

	UFUNCTION(BlueprintPure, Category = "Boss|Target Marker")
	bool ShouldPlayTargetMarker(AActor* TargetActor) const;

	UFUNCTION(BlueprintPure, Category = "Boss|Target Marker")
	bool IsTargetMarkerVFXEnabled() const { return bTargetMarkerVFXEnabled; }

	UFUNCTION(BlueprintCallable, Category = "Boss|Target Marker")
	void SetTargetMarkerVFXEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Boss|Target Marker")
	UNiagaraSystem* GetTargetMarkerSystem() const { return TargetMarkerSystem; }

	UFUNCTION(BlueprintCallable, Category = "Boss|Target Marker")
	void ClearTargetMarkers();

	void HandleOwnerDeath();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayTargetMarker(AActor* TargetActor);

private:
	UFUNCTION(NetMulticast, Reliable)
	void MulticastClearTargetMarkers();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastHandleOwnerDeath();

	void PlayTargetMarkerLocal(AActor* TargetActor);
	USceneComponent* ResolveTargetAttachComponent(AActor* TargetActor) const;
	bool HasTargetBodySocket(AActor* TargetActor) const;
	FVector ResolveTargetBodyLocation(AActor* TargetActor) const;
	bool IsOwnerAllowedToPlayMarkers() const;
	void RegisterTargetMarkerComponent(UNiagaraComponent* MarkerComponent);
	void ClearTargetMarkersLocal();
	void PruneStaleTargetMarkers();

	// Designers can disable the marker without removing the component from inherited boss Blueprints.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Target Marker",
		meta = (AllowPrivateAccess = "true", DisplayName = "Target Marker VFX On/Off"))
	bool bTargetMarkerVFXEnabled = true;

	// Default points at the render-on-top stroke VFX; exposing it keeps later visual swaps editor-only.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Target Marker",
		meta = (AllowPrivateAccess = "true", DisplayName = "Target Marker VFX"))
	TObjectPtr<UNiagaraSystem> TargetMarkerSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Target Marker",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	FVector TargetMarkerScale = FVector(1.0f);

	// Prefer a torso socket when the player mesh has one, then fall back to the actor bounds center.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Target Marker",
		meta = (AllowPrivateAccess = "true"))
	FName TargetBodySocketName = TEXT("spine_03");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Target Marker",
		meta = (AllowPrivateAccess = "true", Units = "cm"))
	FVector TargetBodyOffset = FVector::ZeroVector;

	// Spawned markers are attached to the target player, so the boss must keep handles to clean them up on death.
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<UNiagaraComponent>> ActiveTargetMarkerComponents;

	// Reliable death cleanup may arrive before an older unreliable play RPC, so block any late marker playback after death.
	UPROPERTY(Transient)
	bool bTargetMarkerPlaybackStoppedForOwnerDeath = false;

#if WITH_DEV_AUTOMATION_TESTS
	friend class FBossTargetMarkerVFXConfigurationTest;
#endif
};
