#pragma once

#include "CoreMinimal.h"
#include "NiagaraComponent.h"
#include "GP_BossTelegraphVFXComponent.generated.h"

/** Designer-facing Niagara component that packages reusable boss attack telegraph settings. */
UCLASS(ClassGroup = (Boss), meta = (BlueprintSpawnableComponent, DisplayName = "Boss Telegraph VFX"))
class PROJECT_EDEN_API UGP_BossTelegraphVFXComponent : public UNiagaraComponent
{
	GENERATED_BODY()

public:
	UGP_BossTelegraphVFXComponent();

	UFUNCTION(BlueprintCallable, Category = "Boss|Telegraph")
	void PlayTelegraph();

	UFUNCTION(BlueprintCallable, Category = "Boss|Telegraph")
	void StopTelegraph();

	/** Plays the configured Niagara cue only when the designer-facing toggle is enabled and returns its lead time. */
	UFUNCTION(BlueprintCallable, Category = "Boss|Telegraph")
	float PlayEnabledTelegraph();

	UFUNCTION(BlueprintPure, Category = "Boss|Telegraph")
	bool IsTelegraphVFXEnabled() const { return bTelegraphVFXEnabled; }

	UFUNCTION(BlueprintCallable, Category = "Boss|Telegraph")
	void SetTelegraphVFXEnabled(bool bEnabled) { bTelegraphVFXEnabled = bEnabled; }

	UFUNCTION(BlueprintPure, Category = "Boss|Telegraph")
	float GetEnabledTelegraphDuration() const { return bTelegraphVFXEnabled ? GetTelegraphDuration() : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Boss|Telegraph")
	float GetTelegraphDuration() const { return TelegraphDuration; }

	UFUNCTION(BlueprintPure, Category = "Boss|Telegraph")
	float GetUniformVisualScale() const { return UniformVisualScale; }

	UFUNCTION(BlueprintPure, Category = "Boss|Telegraph")
	UNiagaraSystem* GetDefaultTelegraphSystem() const { return DefaultTelegraphSystem; }

protected:
	virtual void OnRegister() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	void ApplyPresentationSettings();
	void PlayTelegraphLocal();

	/** Boss actors already replicate, so the inherited component can fan the cue out to every relevant client. */
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayTelegraph();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Telegraph",
		meta = (AllowPrivateAccess = "true", DisplayName = "Telegraph VFX On/Off"))
	bool bTelegraphVFXEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Telegraph",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float TelegraphDuration = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Telegraph",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.01", UIMin = "0.1", UIMax = "5.0"))
	float UniformVisualScale = 1.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Telegraph",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraSystem> DefaultTelegraphSystem;
};
