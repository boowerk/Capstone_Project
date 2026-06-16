#include "UI/GP_MinimapCaptureActor.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Components/SceneComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UI/GP_MinimapSubsystem.h"

AGP_MinimapCaptureActor::AGP_MinimapCaptureActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("MinimapSceneCapture"));
	SceneCapture->SetupAttachment(SceneRoot);
	SceneCapture->ProjectionType = ECameraProjectionMode::Orthographic;
	SceneCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	SceneCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
	SceneCapture->bCaptureEveryFrame = false;
	SceneCapture->bCaptureOnMovement = false;
	SceneCapture->bAlwaysPersistRenderingState = true;
	SceneCapture->bUseRayTracingIfEnabled = false;
	SceneCapture->LODDistanceFactor = 2.0f;
}

void AGP_MinimapCaptureActor::BeginPlay()
{
	Super::BeginPlay();

	InitializeCapture();

	if (bRegisterWithSubsystem)
	{
		if (UGP_MinimapSubsystem* MinimapSubsystem = GetWorld()->GetSubsystem<UGP_MinimapSubsystem>())
		{
			MinimapSubsystem->RegisterCaptureActor(this);
		}
	}

	if (bStartFollowingPlayer)
	{
		SetFollowTarget(ResolveDefaultFollowTarget());
		CaptureAroundTarget(FollowTargetActor);
	}
	else
	{
		CaptureFullMap(DefaultBoundsActor);
	}
}

void AGP_MinimapCaptureActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (CaptureMode != EGPMinimapCaptureMode::FollowTarget || !IsValid(FollowTargetActor))
	{
		SetFollowTarget(ResolveDefaultFollowTarget());
		if (!IsValid(FollowTargetActor))
		{
			return;
		}
	}

	FollowCaptureAccumulator += DeltaSeconds;
	if (FollowCaptureAccumulator < FollowCaptureInterval)
	{
		return;
	}

	FollowCaptureAccumulator = 0.0f;
	CaptureForCurrentMode();
}

void AGP_MinimapCaptureActor::InitializeCapture()
{
	if (bCaptureInitialized && IsValid(RenderTarget) && SceneCapture && SceneCapture->TextureTarget == RenderTarget)
	{
		return;
	}

	if (!IsValid(RenderTarget))
	{
		RenderTarget = NewObject<UTextureRenderTarget2D>(this, TEXT("GeneratedMinimapRenderTarget"));
		RenderTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
		RenderTarget->ClearColor = FLinearColor::Transparent;
		RenderTarget->InitAutoFormat(RenderTargetSize, RenderTargetSize);
		RenderTarget->UpdateResourceImmediate(true);
	}

	if (SceneCapture)
	{
		SceneCapture->TextureTarget = RenderTarget;
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
	if (IsValid(BoundsActor))
	{
		DefaultBoundsActor = BoundsActor;
	}

	float DesiredOrthoWidth = FullMapOrthoWidth;
	FVector CaptureCenter = GetActorLocation();

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
	ApplyTopDownTransform(CaptureCenter, DesiredOrthoWidth, 0.0f);
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
	FollowTargetActor = TargetActor;
}

void AGP_MinimapCaptureActor::RequestCapture()
{
	InitializeCapture();

	if (SceneCapture)
	{
		SceneCapture->CaptureScene();
	}

	OnRenderTargetChanged.Broadcast(RenderTarget);
}

FBox AGP_MinimapCaptureActor::ResolveBounds(AActor* BoundsActor) const
{
	if (!IsValid(BoundsActor))
	{
		return FBox(ForceInit);
	}

	// PCG 결과를 예측하지 않고, 현재 월드에 실제 배치된 BoundsActor의 컴포넌트 범위를 사용합니다.
	return BoundsActor->GetComponentsBoundingBox(true);
}

AActor* AGP_MinimapCaptureActor::ResolveDefaultFollowTarget() const
{
	// The player pawn can appear after the HUD or capture actor, so resolve it lazily whenever capture needs it.
	return UGameplayStatics::GetPlayerPawn(this, 0);
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
		const float CaptureYaw = bRotateCaptureWithTarget ? FollowTargetActor->GetActorRotation().Yaw : 0.0f;
		ApplyTopDownTransform(FollowTargetActor->GetActorLocation(), FollowOrthoWidth, CaptureYaw);
		RequestCapture();
		return;
	}

	if (CaptureMode == EGPMinimapCaptureMode::FollowTarget)
	{
		SetFollowTarget(ResolveDefaultFollowTarget());
		if (IsValid(FollowTargetActor))
		{
			const float CaptureYaw = bRotateCaptureWithTarget ? FollowTargetActor->GetActorRotation().Yaw : 0.0f;
			ApplyTopDownTransform(FollowTargetActor->GetActorLocation(), FollowOrthoWidth, CaptureYaw);
			RequestCapture();
			return;
		}
	}

	CaptureFullMap(DefaultBoundsActor);
}
