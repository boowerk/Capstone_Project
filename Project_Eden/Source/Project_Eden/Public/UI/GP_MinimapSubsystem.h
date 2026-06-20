#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GP_MinimapSubsystem.generated.h"

class AGP_MinimapCaptureActor;
class UTextureRenderTarget2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGPMinimapRenderTargetChanged, UTextureRenderTarget2D*, RenderTarget);

UCLASS()
class PROJECT_EDEN_API UGP_MinimapSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void RegisterCaptureActor(AGP_MinimapCaptureActor* CaptureActor);

	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void CaptureFullMap(AActor* BoundsActor = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void NotifyPcgLayoutReady(float CaptureDelay = 0.25f);

	UFUNCTION(BlueprintPure, Category = "Minimap")
	AGP_MinimapCaptureActor* GetActiveCaptureActor();

	UFUNCTION(BlueprintPure, Category = "Minimap")
	UTextureRenderTarget2D* GetMinimapRenderTarget();

	UPROPERTY(BlueprintAssignable, Category = "Minimap")
	FGPMinimapRenderTargetChanged OnRenderTargetChanged;

private:
	AGP_MinimapCaptureActor* ResolveCaptureActor();
	AGP_MinimapCaptureActor* SpawnDefaultCaptureActor();
	void RefreshCurrentCapture();
	void BroadcastCurrentRenderTarget();

	TWeakObjectPtr<AGP_MinimapCaptureActor> ActiveCaptureActor;
	FTimerHandle DelayedCaptureTimerHandle;
};
