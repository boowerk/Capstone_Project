#include "UI/GP_MinimapSubsystem.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "UI/GP_MinimapCaptureActor.h"

void UGP_MinimapSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DelayedCaptureTimerHandle);
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
	BroadcastCurrentRenderTarget();
}

void UGP_MinimapSubsystem::NotifyPcgLayoutReady(float CaptureDelay)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(DelayedCaptureTimerHandle);

	if (CaptureDelay <= 0.0f)
	{
		CaptureFullMap(nullptr);
		return;
	}

	TWeakObjectPtr<UGP_MinimapSubsystem> WeakSubsystem(this);
	World->GetTimerManager().SetTimer(
		DelayedCaptureTimerHandle,
		[WeakSubsystem]()
		{
			if (WeakSubsystem.IsValid())
			{
				// PCG 스폰이 한 프레임 이상 늦게 반영될 수 있어 지연 후 현재 월드를 다시 캡처합니다.
				WeakSubsystem->CaptureFullMap(nullptr);
			}
		},
		CaptureDelay,
		false);
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
		BroadcastCurrentRenderTarget();
		return *It;
	}

	return nullptr;
}

void UGP_MinimapSubsystem::BroadcastCurrentRenderTarget()
{
	if (ActiveCaptureActor.IsValid())
	{
		OnRenderTargetChanged.Broadcast(ActiveCaptureActor->GetMinimapRenderTarget());
	}
}
