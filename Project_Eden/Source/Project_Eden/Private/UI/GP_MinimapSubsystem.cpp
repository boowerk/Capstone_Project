#include "UI/GP_MinimapSubsystem.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
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
		RefreshCurrentCapture();
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
				// Refresh the current mode after PCG settles; forcing FullMap here used to break player-follow capture.
				WeakSubsystem->RefreshCurrentCapture();
			}
		},
		CaptureDelay,
		false);
}

void UGP_MinimapSubsystem::RefreshCurrentCapture()
{
	if (AGP_MinimapCaptureActor* CaptureActor = ResolveCaptureActor())
	{
		CaptureActor->RequestCapture();
	}
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
	CaptureActor->InitializeCapture();
	if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0))
	{
		CaptureActor->CaptureAroundTarget(PlayerPawn);
	}
	else
	{
		CaptureActor->RequestCapture();
	}

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
