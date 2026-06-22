#include "UI/GP_MinimapCaptureActor.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Components/SceneComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "PCGComponent.h"
#include "RHI.h"
#include "RHICommandList.h"
#include "RHIUtilities.h"
#include "RenderingThread.h"
#include "UI/GP_MinimapSubsystem.h"

namespace
{
	struct FGPMinimapCaptureGPUFenceState
	{
		FGPUFenceRHIRef Fence;
		TAtomic<bool> bWriteIssued { false };
	};
}

class FGPMinimapCaptureGPUFence
{
public:
	bool ArmAfterQueuedWork()
	{
		TSharedRef<FGPMinimapCaptureGPUFenceState, ESPMode::ThreadSafe> NewState =
			MakeShared<FGPMinimapCaptureGPUFenceState, ESPMode::ThreadSafe>();
		NewState->Fence = RHICreateGPUFence(TEXT("MinimapCaptureCompletion"));
		if (!NewState->Fence.IsValid())
		{
			State.Reset();
			return false;
		}

		State = NewState;
		if (GUsingNullRHI)
		{
			// NullRHI has no GPU command context for WriteGPUFence; automation drives completion through the test override.
			return true;
		}

		ENQUEUE_RENDER_COMMAND(WriteMinimapCaptureGPUFence)(
			[NewState](FRHICommandListImmediate& RHICmdList)
			{
				// This command is queued after CaptureScene, so signaling covers all prior capture GPU work.
				RHICmdList.WriteGPUFence(NewState->Fence);
				NewState->bWriteIssued.Store(true);
			});
		return true;
	}

	bool IsComplete() const
	{
		return State.IsValid()
			&& State->bWriteIssued.Load()
			&& State->Fence.IsValid()
			&& State->Fence->NumPendingWriteCommands.GetValue() == 0
			&& State->Fence->Poll();
	}

	bool HasFence() const
	{
		return State.IsValid() && State->Fence.IsValid();
	}

	void Reset()
	{
		State.Reset();
	}

private:
	TSharedPtr<FGPMinimapCaptureGPUFenceState, ESPMode::ThreadSafe> State;
};

AGP_MinimapCaptureActor::AGP_MinimapCaptureActor()
	: CaptureCompletionFence(MakeShared<FGPMinimapCaptureGPUFence, ESPMode::ThreadSafe>())
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("MinimapSceneCapture"));
	SceneCapture->SetupAttachment(SceneRoot);
	SceneCapture->ProjectionType = ECameraProjectionMode::Orthographic;
	SceneCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
	SceneCapture->bCaptureEveryFrame = false;
	SceneCapture->bCaptureOnMovement = false;
	SceneCapture->bAlwaysPersistRenderingState = true;
	SceneCapture->bUseRayTracingIfEnabled = false;
	SceneCapture->LODDistanceFactor = 2.0f;
	ConfigureFlat2DCapture();
}

AGP_MinimapCaptureActor::~AGP_MinimapCaptureActor() = default;

void AGP_MinimapCaptureActor::BeginPlay()
{
	Super::BeginPlay();

	CacheInitialGroundCenter();
	InitializeCapture();

	if (bRegisterWithSubsystem)
	{
		if (UGP_MinimapSubsystem* MinimapSubsystem = GetWorld()->GetSubsystem<UGP_MinimapSubsystem>())
		{
			MinimapSubsystem->RegisterCaptureActor(this);
		}
	}

	// PCG owns the one-shot capture timing; BeginPlay only prepares the stable HUD texture.
}

void AGP_MinimapCaptureActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AdvanceFrameTransfer();
}

void AGP_MinimapCaptureActor::InitializeCapture()
{
	ConfigureFlat2DCapture();

	if (bCaptureInitialized && IsValid(RenderTarget) && IsValid(CaptureBackBuffer))
	{
		return;
	}

	if (!IsValid(RenderTarget))
	{
		RenderTarget = CreateTransientRenderTarget(TEXT("GeneratedMinimapRenderTargetFront"));
	}
	if (!IsValid(CaptureBackBuffer))
	{
		CaptureBackBuffer = CreateTransientRenderTarget(TEXT("GeneratedMinimapRenderTargetBack"));
	}

	if (SceneCapture)
	{
		// Never capture into the texture currently sampled by UMG.
		SceneCapture->TextureTarget = CaptureBackBuffer;
		SceneCapture->ProjectionType = ECameraProjectionMode::Orthographic;
		SceneCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
		SceneCapture->bCaptureEveryFrame = false;
		SceneCapture->bCaptureOnMovement = false;
		SceneCapture->bAlwaysPersistRenderingState = true;
		SceneCapture->MaxViewDistanceOverride = CaptureHeight * 2.0f;
	}

	bCaptureInitialized = true;
	OnRenderTargetChanged.Broadcast(RenderTarget);
}

void AGP_MinimapCaptureActor::CaptureFullMap(AActor* BoundsActor)
{
	InitializeCapture();

	CaptureMode = EGPMinimapCaptureMode::FullMap;
	bFullMapCaptureReady = false;
	bFullMapCapturePending = true;
	if (IsValid(BoundsActor))
	{
		DefaultBoundsActor = BoundsActor;
	}

	float DesiredOrthoWidth = FullMapOrthoWidth;
	FVector CaptureCenter = ResolveFallbackFullMapCenter();

	const FBox BoundsBox = ResolveBounds(DefaultBoundsActor);
	if (BoundsBox.IsValid)
	{
		const FVector BoundsExtent = BoundsBox.GetExtent();
		CaptureCenter = BoundsBox.GetCenter();
		const float BoundsWidthX = static_cast<float>(BoundsExtent.X * 2.0);
		const float BoundsWidthY = static_cast<float>(BoundsExtent.Y * 2.0);
		DesiredOrthoWidth = FMath::Max3(BoundsWidthX, BoundsWidthY, FullMapOrthoWidth) + BoundsPadding;
	}

	// 전체 지도는 북쪽 고정 캡처를 유지해 플레이어 화살표 회전과 기준축이 안정적으로 맞습니다.
	// Store the exact camera mapping so HUD UV conversion uses the same center, width, and yaw as this capture.
	CapturedMapCenter = CaptureCenter;
	CapturedMapOrthoWidth = DesiredOrthoWidth;
	CapturedMapYaw = 0.0f;
	ApplyTopDownTransform(CaptureCenter, DesiredOrthoWidth, CapturedMapYaw);
	RequestCapture();
}

void AGP_MinimapCaptureActor::CaptureAroundTarget(AActor* TargetActor)
{
	SetFollowTarget(TargetActor);
	CaptureMode = EGPMinimapCaptureMode::FollowTarget;
	CaptureForCurrentMode();
}

void AGP_MinimapCaptureActor::SetFollowTarget(AActor* TargetActor)
{
	if (SceneCapture && IsValid(FollowTargetActor))
	{
		SceneCapture->HiddenActors.Remove(FollowTargetActor);
	}

	FollowTargetActor = TargetActor;
	if (SceneCapture && IsValid(FollowTargetActor))
	{
		// The HUD arrow represents the player; hiding the pawn also prevents attack meshes from covering the map capture.
		SceneCapture->HiddenActors.AddUnique(FollowTargetActor);
	}
}

void AGP_MinimapCaptureActor::RequestCapture()
{
	InitializeCapture();
	SetActorTickEnabled(true);
	AdvanceFrameTransfer();

	if (FrameTransferState != EFrameTransferState::Idle || !SceneCapture || !IsValid(CaptureBackBuffer))
	{
		return;
	}

	// Reactivate only for an explicit capture request; the component sleeps after the one-shot GPU copy completes.
	SceneCapture->Activate();
	SceneCapture->TextureTarget = CaptureBackBuffer;
	SceneCapture->CaptureScene();
	// The GPU fence is written after CaptureScene; keep the existing front buffer if the fence cannot be armed.
	if (CaptureCompletionFence.IsValid() && CaptureCompletionFence->ArmAfterQueuedWork())
	{
		FrameTransferState = EFrameTransferState::WaitingForCapture;
	}
}

FBox AGP_MinimapCaptureActor::ResolveBounds(AActor* BoundsActor) const
{
	if (IsValid(BoundsActor))
	{
		return BoundsActor->GetComponentsBoundingBox(true);
	}

	// PCG 결과를 예측하지 않고, 현재 월드에 실제 배치된 BoundsActor의 컴포넌트 범위를 사용합니다.
	FBox GeneratedBounds(ForceInit);
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->FindComponentByClass<UPCGComponent>())
			{
				// Generated ISM components live on PCG owners, so their runtime bounds define the map footprint.
				GeneratedBounds += It->GetComponentsBoundingBox(true);
			}
		}
	}

	return GeneratedBounds;
}

bool AGP_MinimapCaptureActor::WorldToMapUV(const FVector& WorldLocation, FVector2D& OutMapUV) const
{
	if (CapturedMapOrthoWidth <= KINDA_SMALL_NUMBER)
	{
		OutMapUV = FVector2D(0.5f, 0.5f);
		return false;
	}

	const FRotator CaptureRotation(-90.0f, CapturedMapYaw, 0.0f);
	const FVector ViewRight = CaptureRotation.RotateVector(FVector::RightVector);
	const FVector ViewUp = CaptureRotation.RotateVector(FVector::UpVector);
	const FVector Delta = WorldLocation - CapturedMapCenter;

	// U grows along camera-right while texture V grows opposite camera-up.
	OutMapUV.X = 0.5f + (FVector::DotProduct(Delta, ViewRight) / CapturedMapOrthoWidth);
	OutMapUV.Y = 0.5f - (FVector::DotProduct(Delta, ViewUp) / CapturedMapOrthoWidth);
	return true;
}

AActor* AGP_MinimapCaptureActor::ResolveDefaultFollowTarget() const
{
	// The player pawn can appear after the HUD or capture actor, so resolve it lazily whenever capture needs it.
	return UGameplayStatics::GetPlayerPawn(this, 0);
}

FVector AGP_MinimapCaptureActor::ResolveFallbackFullMapCenter()
{
	if (IsValid(FollowTargetActor))
	{
		return FollowTargetActor->GetActorLocation();
	}

	if (AActor* DefaultFollowTarget = ResolveDefaultFollowTarget())
	{
		return DefaultFollowTarget->GetActorLocation();
	}

	CacheInitialGroundCenter();
	return InitialGroundCenter;
}

void AGP_MinimapCaptureActor::CacheInitialGroundCenter()
{
	if (bHasInitialGroundCenter)
	{
		return;
	}

	// Cache the authored/spawned ground center once; never derive a new center from the already elevated capture camera.
	InitialGroundCenter = GetActorLocation();
	bHasInitialGroundCenter = true;
}

void AGP_MinimapCaptureActor::ConfigureFlat2DCapture()
{
	if (!SceneCapture)
	{
		return;
	}

	// FinalColorLDR preserves opaque alpha for direct UMG display; show flags keep the result flat and shadow-free.
	SceneCapture->ProjectionType = ECameraProjectionMode::Orthographic;
	SceneCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	SceneCapture->ShowFlags.SetLighting(false);
	SceneCapture->ShowFlags.SetSkyLighting(false);
	SceneCapture->ShowFlags.SetDynamicShadows(false);
	SceneCapture->ShowFlags.SetPostProcessing(false);
	SceneCapture->ShowFlags.SetAtmosphere(false);
	SceneCapture->ShowFlags.SetAntiAliasing(false);
	SceneCapture->ShowFlags.SetFog(false);
	SceneCapture->ShowFlags.SetVolumetricFog(false);
	SceneCapture->ShowFlags.SetTranslucency(false);
	SceneCapture->ShowFlags.SetParticles(false);
	SceneCapture->ShowFlags.SetDecals(false);
	SceneCapture->ShowFlags.SetBloom(false);
	SceneCapture->ShowFlags.SetAmbientOcclusion(false);
	SceneCapture->ShowFlags.SetMotionBlur(false);
	SceneCapture->ShowFlags.SetBounds(false);
	SceneCapture->ShowFlags.SetCollision(false);
	SceneCapture->ShowFlags.SetDebugAI(false);
	SceneCapture->ShowFlags.SetNavigation(false);
	SceneCapture->PostProcessSettings.bOverride_VignetteIntensity = true;
	SceneCapture->PostProcessSettings.VignetteIntensity = 0.0f;
}

UTextureRenderTarget2D* AGP_MinimapCaptureActor::CreateTransientRenderTarget(const FName ObjectName)
{
	UTextureRenderTarget2D* NewRenderTarget = NewObject<UTextureRenderTarget2D>(this, ObjectName);
	if (!NewRenderTarget)
	{
		return nullptr;
	}

	NewRenderTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
	// The render target is displayed directly by UMG, so keep its fallback alpha opaque.
	NewRenderTarget->ClearColor = FLinearColor(0.015f, 0.015f, 0.015f, 1.0f);
	NewRenderTarget->InitAutoFormat(RenderTargetSize, RenderTargetSize);
	NewRenderTarget->UpdateResourceImmediate(true);
	return NewRenderTarget;
}

void AGP_MinimapCaptureActor::AdvanceFrameTransfer()
{
	if (FrameTransferState == EFrameTransferState::Idle || !IsTransferGPUFenceComplete())
	{
		return;
	}

	if (FrameTransferState == EFrameTransferState::WaitingForCapture)
	{
		if (!QueueCapturedFrameToDisplay())
		{
			// A failed copy leaves the last valid HUD frame untouched and reopens the capture pipeline.
			FrameTransferState = EFrameTransferState::Idle;
			CaptureCompletionFence->Reset();
		}
		return;
	}

	// The HUD remains bound to the same front RenderTarget object, so no Slate brush rebind is required here.
	FrameTransferState = EFrameTransferState::Idle;
	CaptureCompletionFence->Reset();

	if (bFullMapCapturePending)
	{
		bFullMapCapturePending = false;
		bFullMapCaptureReady = true;
		if (SceneCapture)
		{
			// No scene rendering is needed after PCG's completed map reaches the stable display texture.
			SceneCapture->Deactivate();
			SceneCapture->SetComponentTickEnabled(false);
		}
		OnMapCaptureReady.Broadcast();
		SetActorTickEnabled(false);
	}
}

bool AGP_MinimapCaptureActor::QueueCapturedFrameToDisplay()
{
	if (!IsValid(RenderTarget) || !IsValid(CaptureBackBuffer) || !CaptureCompletionFence.IsValid())
	{
		return false;
	}

	if (!GUsingNullRHI)
	{
		FTextureRenderTargetResource* SourceResource = CaptureBackBuffer->GameThread_GetRenderTargetResource();
		FTextureRenderTargetResource* DisplayResource = RenderTarget->GameThread_GetRenderTargetResource();
		if (!SourceResource || !DisplayResource)
		{
			return false;
		}

		ENQUEUE_RENDER_COMMAND(CopyMinimapCaptureToStableDisplay)(
			[SourceResource, DisplayResource](FRHICommandListImmediate& RHICmdList)
			{
				FRHITexture* SourceTexture = SourceResource->GetRenderTargetTexture();
				FRHITexture* DisplayTexture = DisplayResource->GetRenderTargetTexture();
				if (SourceTexture && DisplayTexture)
				{
					// Keep UMG on one stable texture object; only its completed GPU pixels are refreshed.
					TransitionAndCopyTexture(RHICmdList, SourceTexture, DisplayTexture, {});
				}
			});
	}

	CaptureCompletionFence->Reset();
	if (!CaptureCompletionFence->ArmAfterQueuedWork())
	{
		return false;
	}

	FrameTransferState = EFrameTransferState::WaitingForDisplayCopy;
	return true;
}

bool AGP_MinimapCaptureActor::IsTransferGPUFenceComplete() const
{
#if WITH_DEV_AUTOMATION_TESTS
	if (CaptureGPUFenceCompletionOverride.IsSet())
	{
		return CaptureGPUFenceCompletionOverride.GetValue();
	}
#endif
	return CaptureCompletionFence.IsValid() && CaptureCompletionFence->IsComplete();
}

bool AGP_MinimapCaptureActor::HasTransferGPUFence() const
{
	return CaptureCompletionFence.IsValid() && CaptureCompletionFence->HasFence();
}

void AGP_MinimapCaptureActor::ApplyTopDownTransform(const FVector& Center, float OrthoWidth, float Yaw)
{
	const FVector CaptureLocation(Center.X, Center.Y, Center.Z + CaptureHeight);
	SetActorLocation(CaptureLocation);
	SetActorRotation(FRotator(-90.0f, Yaw, 0.0f));

	if (SceneCapture)
	{
		SceneCapture->OrthoWidth = OrthoWidth;
	}
}

void AGP_MinimapCaptureActor::CaptureForCurrentMode()
{
	InitializeCapture();

	if (CaptureMode == EGPMinimapCaptureMode::FollowTarget && IsValid(FollowTargetActor))
	{
		// Legacy manual follow requests remain callable, but no periodic SceneCapture tick is scheduled.
		ApplyTopDownTransform(FollowTargetActor->GetActorLocation(), FollowOrthoWidth, 0.0f);
		RequestCapture();
		return;
	}

	if (CaptureMode == EGPMinimapCaptureMode::FollowTarget)
	{
		SetFollowTarget(ResolveDefaultFollowTarget());
		if (IsValid(FollowTargetActor))
		{
			ApplyTopDownTransform(FollowTargetActor->GetActorLocation(), FollowOrthoWidth, 0.0f);
			RequestCapture();
			return;
		}
	}

	CaptureFullMap(DefaultBoundsActor);
}
