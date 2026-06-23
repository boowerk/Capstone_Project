#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "GP_BossTargetMarkerVFXComponent.generated.h"

class UNiagaraSystem;
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
	void SetTargetMarkerVFXEnabled(bool bEnabled) { bTargetMarkerVFXEnabled = bEnabled; }

	UFUNCTION(BlueprintPure, Category = "Boss|Target Marker")
	UNiagaraSystem* GetTargetMarkerSystem() const { return TargetMarkerSystem; }

protected:
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayTargetMarker(AActor* TargetActor);

private:
	void PlayTargetMarkerLocal(AActor* TargetActor) const;
	USceneComponent* ResolveTargetAttachComponent(AActor* TargetActor) const;
	bool HasTargetBodySocket(AActor* TargetActor) const;
	FVector ResolveTargetBodyLocation(AActor* TargetActor) const;

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
};
