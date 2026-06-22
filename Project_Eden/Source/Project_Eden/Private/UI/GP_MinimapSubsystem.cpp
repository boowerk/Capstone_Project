#include "UI/GP_MinimapSubsystem.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "PCGComponent.h"
#include "TimerManager.h"
#include "UI/GP_MinimapCaptureActor.h"

void UGP_MinimapSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DelayedCaptureTimerHandle);
	}
	if (ActiveCaptureActor.IsValid())
	{
		ActiveCaptureActor->OnMapCaptureReady.RemoveDynamic(this, &ThisClass::HandleMapCaptureReady);
	}

	ActiveCaptureActor.Reset();
	Super::Deinitialize();
}

void UGP_MinimapSubsystem::RegisterCaptureActor(AGP_MinimapCaptureActor* CaptureActor)
{
	if (!IsValid(CaptureActor))
	{
		return;
	}

	ActiveCaptureActor = CaptureActor;
	CaptureActor->OnMapCaptureReady.AddUniqueDynamic(this, &ThisClass::HandleMapCaptureReady);
	BroadcastCurrentRenderTarget();
}

void UGP_MinimapSubsystem::CaptureFullMap(AActor* BoundsActor)
{
	AGP_MinimapCaptureActor* CaptureActor = ResolveCaptureActor();
	if (!IsValid(CaptureActor))
	{
		return;
	}

	CaptureActor->CaptureFullMap(BoundsActor);
}

void UGP_MinimapSubsystem::NotifyPcgLayoutReady(float CaptureDelay)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(DelayedCaptureTimerHandle);
	if (bPcgLayoutCaptureRequested)
	{
		return;
	}
	bPcgLayoutCaptureRequested = true;

	if (CaptureDelay <= 0.0f)
	{
		CapturePcgLayoutOnce();
		return;
	}

	TWeakObjectPtr<UGP_MinimapSubsystem> WeakSubsystem(this);
	World->GetTimerManager().SetTimer(
		DelayedCaptureTimerHandle,
		[WeakSubsystem]()
		{
			if (WeakSubsystem.IsValid())
			{
				// Delay lets asynchronous PCG instances finish registering before the single full-map capture.
				WeakSubsystem->CapturePcgLayoutOnce();
			}
		},
		CaptureDelay,
		false);
}

void UGP_MinimapSubsystem::CapturePcgLayoutOnce()
{
	const bool bPcgStillGenerating = ArePcgComponentsStillGenerating();
	ConsecutivePcgIdlePolls = bPcgStillGenerating ? 0 : ConsecutivePcgIdlePolls + 1;
	if (bPcgStillGenerating || ConsecutivePcgIdlePolls < 3)
	{
		if (UWorld* World = GetWorld())
		{
			TWeakObjectPtr<UGP_MinimapSubsystem> WeakSubsystem(this);
			World->GetTimerManager().SetTimer(
				DelayedCaptureTimerHandle,
				[WeakSubsystem]()
				{
					if (WeakSubsystem.IsValid())
					{
						WeakSubsystem->CapturePcgLayoutOnce();
					}
				},
				0.1f,
				false);
		}
		// Requiring three quiet polls avoids capturing in the short gap before deferred PCG tasks begin.
		return;
	}

	if (AGP_MinimapCaptureActor* CaptureActor = ResolveCaptureActor())
	{
		CaptureActor->CaptureFullMap(nullptr);
	}
}

bool UGP_MinimapSubsystem::ArePcgComponentsStillGenerating() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		TInlineComponentArray<UPCGComponent*> PcgComponents(*It);
		for (const UPCGComponent* PcgComponent : PcgComponents)
		{
			if (IsValid(PcgComponent) && PcgComponent->IsGenerating())
			{
				// Poll only during startup so capture occurs after every active PCG task has finished.
				return true;
			}
		}
	}

	return false;
}

AGP_MinimapCaptureActor* UGP_MinimapSubsystem::GetActiveCaptureActor()
{
	return ResolveCaptureActor();
}

UTextureRenderTarget2D* UGP_MinimapSubsystem::GetMinimapRenderTarget()
{
	if (AGP_MinimapCaptureActor* CaptureActor = ResolveCaptureActor())
	{
		return CaptureActor->GetMinimapRenderTarget();
	}

	return nullptr;
}

bool UGP_MinimapSubsystem::WorldToMapUV(const FVector& WorldLocation, FVector2D& OutMapUV)
{
	if (AGP_MinimapCaptureActor* CaptureActor = ResolveCaptureActor())
	{
		return CaptureActor->WorldToMapUV(WorldLocation, OutMapUV);
	}

	OutMapUV = FVector2D(0.5f, 0.5f);
	return false;
}

bool UGP_MinimapSubsystem::IsMinimapReady()
{
	if (AGP_MinimapCaptureActor* CaptureActor = ResolveCaptureActor())
	{
		return CaptureActor->IsFullMapCaptureReady();
	}

	return false;
}

AGP_MinimapCaptureActor* UGP_MinimapSubsystem::ResolveCaptureActor()
{
	if (ActiveCaptureActor.IsValid())
	{
		return ActiveCaptureActor.Get();
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<AGP_MinimapCaptureActor> It(World); It; ++It)
	{
		ActiveCaptureActor = *It;
		It->OnMapCaptureReady.AddUniqueDynamic(this, &ThisClass::HandleMapCaptureReady);
		BroadcastCurrentRenderTarget();
		return *It;
	}

	return SpawnDefaultCaptureActor();
}

AGP_MinimapCaptureActor* UGP_MinimapSubsystem::SpawnDefaultCaptureActor()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParameters.ObjectFlags |= RF_Transient;

	AGP_MinimapCaptureActor* CaptureActor = World->SpawnActor<AGP_MinimapCaptureActor>(
		AGP_MinimapCaptureActor::StaticClass(),
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParameters);

	if (!IsValid(CaptureActor))
	{
		return nullptr;
	}

	// Fallback spawn keeps the minimap alive even when the level forgot to place a capture actor.
	ActiveCaptureActor = CaptureActor;
	CaptureActor->OnMapCaptureReady.AddUniqueDynamic(this, &ThisClass::HandleMapCaptureReady);
	CaptureActor->InitializeCapture();
	BroadcastCurrentRenderTarget();
	return CaptureActor;
}

void UGP_MinimapSubsystem::BroadcastCurrentRenderTarget()
{
	if (ActiveCaptureActor.IsValid())
	{
		OnRenderTargetChanged.Broadcast(ActiveCaptureActor->GetMinimapRenderTarget());
	}
}

void UGP_MinimapSubsystem::HandleMapCaptureReady()
{
	// The render-target object stays stable; this event only announces completed one-shot PCG pixels.
	OnMinimapReady.Broadcast();
}
