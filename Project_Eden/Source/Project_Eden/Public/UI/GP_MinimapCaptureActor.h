#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP_MinimapCaptureActor.generated.h"

class USceneCaptureComponent2D;
class USceneComponent;
class UTextureRenderTarget2D;
class FGPMinimapCaptureGPUFence;
class FMinimapCaptureStabilityTest;

UENUM(BlueprintType)
enum class EGPMinimapCaptureMode : uint8
{
	FullMap,
	FollowTarget
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGPMinimapCaptureRenderTargetChanged, UTextureRenderTarget2D*, RenderTarget);

UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_MinimapCaptureActor : public AActor
{
	GENERATED_BODY()

public:
	AGP_MinimapCaptureActor();
	virtual ~AGP_MinimapCaptureActor() override;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void InitializeCapture();

	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void CaptureFullMap(AActor* BoundsActor = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void CaptureAroundTarget(AActor* TargetActor);

	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetFollowTarget(AActor* TargetActor);

	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void RequestCapture();

	UFUNCTION(BlueprintPure, Category = "Minimap")
	UTextureRenderTarget2D* GetMinimapRenderTarget() const { return RenderTarget; }

	UPROPERTY(BlueprintAssignable, Category = "Minimap")
	FGPMinimapCaptureRenderTargetChanged OnRenderTargetChanged;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minimap")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minimap")
	TObjectPtr<USceneCaptureComponent2D> SceneCapture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Capture")
	TObjectPtr<UTextureRenderTarget2D> RenderTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Capture", meta = (ClampMin = "64", ClampMax = "4096"))
	int32 RenderTargetSize = 1024;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Capture")
	TObjectPtr<AActor> DefaultBoundsActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Capture", meta = (ClampMin = "100.0"))
	float FullMapOrthoWidth = 8000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Capture", meta = (ClampMin = "100.0"))
	float FollowOrthoWidth = 2200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Capture", meta = (ClampMin = "100.0"))
	float CaptureHeight = 6000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Capture", meta = (ClampMin = "0.0"))
	float BoundsPadding = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Capture", meta = (ClampMin = "0.02"))
	float FollowCaptureInterval = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Capture")
	bool bRegisterWithSubsystem = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Capture")
	bool bStartFollowingPlayer = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Capture")
	bool bRotateCaptureWithTarget = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Capture")
	EGPMinimapCaptureMode CaptureMode = EGPMinimapCaptureMode::FollowTarget;

private:
	friend class FMinimapCaptureStabilityTest;

	FBox ResolveBounds(AActor* BoundsActor) const;
	AActor* ResolveDefaultFollowTarget() const;
	FVector ResolveFallbackFullMapCenter();
	void CacheInitialGroundCenter();
	void ConfigureFlat2DCapture();
	UTextureRenderTarget2D* CreateTransientRenderTarget(const FName ObjectName);
	bool IsCaptureGPUFenceComplete() const;
	bool HasCaptureGPUFence() const;
	void PromoteCompletedCapture();
	void ApplyTopDownTransform(const FVector& Center, float OrthoWidth, float Yaw);
	void CaptureForCurrentMode();

	UPROPERTY(Transient)
	TObjectPtr<AActor> FollowTargetActor;

	// SceneCapture writes only to this back buffer; RenderTarget remains stable for UMG until promotion.
	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> CaptureBackBuffer;

	float FollowCaptureAccumulator = 0.0f;
	FVector InitialGroundCenter = FVector::ZeroVector;
	// The GPU fence prevents promotion while SceneCapture or Niagara work is still filling CaptureBackBuffer.
	TSharedPtr<FGPMinimapCaptureGPUFence, ESPMode::ThreadSafe> CaptureCompletionFence;
#if WITH_DEV_AUTOMATION_TESTS
	TOptional<bool> CaptureGPUFenceCompletionOverride;
#endif
	bool bHasInitialGroundCenter = false;
	bool bCaptureInitialized = false;
	bool bHasPendingCapture = false;
};
