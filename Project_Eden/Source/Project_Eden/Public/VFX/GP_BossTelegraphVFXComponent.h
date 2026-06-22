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
