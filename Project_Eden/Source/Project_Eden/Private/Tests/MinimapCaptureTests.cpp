#if WITH_DEV_AUTOMATION_TESTS

#include "Components/SceneCaptureComponent2D.h"
#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "Components/Overlay.h"
#include "Components/Widget.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "Misc/AutomationTest.h"
#include "UI/GP_MinimapCaptureActor.h"
#include "UI/GP_MinimapSubsystem.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"

namespace
{
	bool IsLikelyProductionMinimapMapImageName(const FString& WidgetName)
	{
		const bool bMentionsMinimap = WidgetName.Contains(TEXT("Minimap"), ESearchCase::IgnoreCase);
		const bool bMentionsMapBackground =
			WidgetName.Contains(TEXT("Background"), ESearchCase::IgnoreCase)
			|| WidgetName.Contains(TEXT("MapImage"), ESearchCase::IgnoreCase);
		const bool bLooksLikeOverlayOnly =
			WidgetName.Contains(TEXT("Arrow"), ESearchCase::IgnoreCase)
			|| WidgetName.Contains(TEXT("Marker"), ESearchCase::IgnoreCase)
			|| WidgetName.Contains(TEXT("Point"), ESearchCase::IgnoreCase)
			|| WidgetName.Contains(TEXT("Ring"), ESearchCase::IgnoreCase)
			|| WidgetName.Contains(TEXT("Backplate"), ESearchCase::IgnoreCase);

		// Match the runtime resolver so the test follows intentional BP widget renames.
		return bMentionsMinimap && bMentionsMapBackground && !bLooksLikeOverlayOnly;
	}

	UImage* ResolveProductionMinimapMapImage(UWidgetTree* WidgetTree)
	{
		if (!WidgetTree)
		{
			return nullptr;
		}

		static const FName CandidateNames[] =
		{
			TEXT("MinimapBackgroundImage"),
			TEXT("MiniMapBackgroundImage"),
			TEXT("MinimapBackground"),
			TEXT("MiniMapBackground"),
			TEXT("MinimapImage"),
			TEXT("MiniMapImage"),
			TEXT("MinimapMapImage"),
			TEXT("MiniMapMapImage"),
			TEXT("MinimapRenderTargetImage"),
			TEXT("MiniMapRenderTargetImage"),
			TEXT("MapBackgroundImage"),
			TEXT("MapBackground"),
			TEXT("MapImage")
		};

		for (const FName& CandidateName : CandidateNames)
		{
			if (UImage* CandidateImage = Cast<UImage>(WidgetTree->FindWidget(CandidateName)))
			{
				return CandidateImage;
			}
		}

		UImage* FoundImage = nullptr;
		WidgetTree->ForEachWidget([&FoundImage](UWidget* ChildWidget)
		{
			if (FoundImage || !ChildWidget)
			{
				return;
			}

			if (IsLikelyProductionMinimapMapImageName(ChildWidget->GetName()))
			{
				FoundImage = Cast<UImage>(ChildWidget);
			}
		});

		return FoundImage;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMinimapCaptureStabilityTest,
	"ProjectEden.UI.Minimap.CaptureStability",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMinimapCaptureStabilityTest::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Created transient minimap test world"), TestWorld))
	{
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGP_MinimapCaptureActor* CaptureActor = TestWorld->SpawnActor<AGP_MinimapCaptureActor>(
		FVector(100.0f, 200.0f, 300.0f),
		FRotator::ZeroRotator,
		SpawnParameters);
	if (!TestNotNull(TEXT("Spawned minimap capture actor"), CaptureActor))
	{
		TestWorld->DestroyWorld(false);
		return false;
	}

	USceneCaptureComponent2D* SceneCapture = CaptureActor->SceneCapture;
	TestNotNull(TEXT("Minimap owns a scene capture component"), SceneCapture);
	if (SceneCapture)
	{
		TestEqual(TEXT("Minimap uses orthographic projection"), SceneCapture->ProjectionType, ECameraProjectionMode::Orthographic);
		TestEqual(TEXT("Minimap captures opaque UI color"), SceneCapture->CaptureSource, ESceneCaptureSource::SCS_FinalColorLDR);
		TestFalse(TEXT("Minimap capture excludes lighting"), SceneCapture->ShowFlags.Lighting);
		TestFalse(TEXT("Minimap capture excludes shadows"), SceneCapture->ShowFlags.DynamicShadows);
		TestFalse(TEXT("Minimap capture excludes particles"), SceneCapture->ShowFlags.Particles);
	}

	CaptureActor->InitializeCapture();
	TestNotNull(TEXT("Minimap creates a display render target"), CaptureActor->RenderTarget.Get());
	TestNotNull(TEXT("Minimap creates a separate capture back buffer"), CaptureActor->CaptureBackBuffer.Get());
	TestNotEqual(
		TEXT("Scene capture never writes into the texture currently displayed by UMG"),
		CaptureActor->RenderTarget.Get(),
		CaptureActor->CaptureBackBuffer.Get());
	if (SceneCapture)
	{
		// Compare the resolved pointers explicitly because TObjectPtr and raw-pointer template deduction differs by engine version.
		TestTrue(TEXT("Scene capture targets the back buffer"), SceneCapture->TextureTarget.Get() == CaptureActor->CaptureBackBuffer.Get());
	}

	UGP_MinimapSubsystem* MinimapSubsystem = TestWorld->GetSubsystem<UGP_MinimapSubsystem>();
	TestNotNull(TEXT("Transient world owns a minimap subsystem"), MinimapSubsystem);
	if (MinimapSubsystem)
	{
		MinimapSubsystem->RegisterCaptureActor(CaptureActor);
		TestTrue(TEXT("Registering the capture actor schedules the no-PCG fallback capture"), MinimapSubsystem->bInitialCaptureScheduled);
		TestTrue(TEXT("Fallback capture request remains pending until a completed map frame is promoted"), MinimapSubsystem->bPcgLayoutCaptureRequested);
		MinimapSubsystem->NotifyPcgLayoutReady(0.0f);
		TestTrue(TEXT("Explicit PCG-ready notification is not ignored while fallback is pending"), MinimapSubsystem->bPcgLayoutCaptureRequested);
	}

	UTextureRenderTarget2D* StableFrontBuffer = CaptureActor->RenderTarget.Get();
	UTextureRenderTarget2D* PendingBackBuffer = CaptureActor->CaptureBackBuffer.Get();
	CaptureActor->CaptureFullMap();
	TestTrue(TEXT("Minimap capture arms an RHI GPU fence"), CaptureActor->HasTransferGPUFence());
	TestTrue(
		TEXT("Minimap waits for the back-buffer capture"),
		CaptureActor->FrameTransferState == AGP_MinimapCaptureActor::EFrameTransferState::WaitingForCapture);

	CaptureActor->CaptureGPUFenceCompletionOverride = false;
	CaptureActor->AdvanceFrameTransfer();
	TestTrue(TEXT("Incomplete GPU capture keeps the existing HUD render target"), CaptureActor->RenderTarget.Get() == StableFrontBuffer);
	TestTrue(TEXT("Incomplete GPU capture keeps the same back buffer pending"), CaptureActor->CaptureBackBuffer.Get() == PendingBackBuffer);
	TestTrue(
		TEXT("Incomplete GPU capture remains pending"),
		CaptureActor->FrameTransferState == AGP_MinimapCaptureActor::EFrameTransferState::WaitingForCapture);

	CaptureActor->CaptureGPUFenceCompletionOverride = true;
	CaptureActor->AdvanceFrameTransfer();
	TestTrue(TEXT("Completed capture keeps the HUD bound to its stable render target"), CaptureActor->RenderTarget.Get() == StableFrontBuffer);
	TestTrue(TEXT("Completed capture keeps SceneCapture isolated on the back buffer"), SceneCapture->TextureTarget.Get() == PendingBackBuffer);
	TestTrue(
		TEXT("Completed capture waits for the display copy fence"),
		CaptureActor->FrameTransferState == AGP_MinimapCaptureActor::EFrameTransferState::WaitingForDisplayCopy);

	CaptureActor->CaptureGPUFenceCompletionOverride = false;
	// A new follow update must not reuse the back buffer while its previous frame is still copying to the HUD.
	CaptureActor->RequestCapture();
	TestTrue(
		TEXT("Incomplete display copy keeps the capture pipeline occupied"),
		CaptureActor->FrameTransferState == AGP_MinimapCaptureActor::EFrameTransferState::WaitingForDisplayCopy);
	TestTrue(TEXT("Incomplete display copy never rebinds the HUD texture"), CaptureActor->RenderTarget.Get() == StableFrontBuffer);
	TestTrue(TEXT("Blocked recapture preserves the isolated back buffer"), SceneCapture->TextureTarget.Get() == PendingBackBuffer);

	CaptureActor->CaptureGPUFenceCompletionOverride = true;
	CaptureActor->AdvanceFrameTransfer();
	TestTrue(
		TEXT("Completed display copy reopens the capture pipeline"),
		CaptureActor->FrameTransferState == AGP_MinimapCaptureActor::EFrameTransferState::Idle);
	TestTrue(TEXT("Display completion still keeps the original HUD texture object"), CaptureActor->RenderTarget.Get() == StableFrontBuffer);
	TestTrue(TEXT("One-shot full-map pixels are marked ready after the display copy"), CaptureActor->IsFullMapCaptureReady());
	TestFalse(TEXT("One-shot capture actor disables its tick after completion"), CaptureActor->IsActorTickEnabled());
	if (SceneCapture)
	{
		TestFalse(TEXT("SceneCapture is inactive after the PCG map is copied"), SceneCapture->IsActive());
	}
	CaptureActor->CaptureGPUFenceCompletionOverride.Reset();

	FVector2D CenterUV;
	TestTrue(TEXT("Captured map center can be converted to UV"), CaptureActor->WorldToMapUV(CaptureActor->CapturedMapCenter, CenterUV));
	TestTrue(TEXT("Captured map center resolves to texture center"), CenterUV.Equals(FVector2D(0.5f, 0.5f), KINDA_SMALL_NUMBER));

	const FRotator CapturedRotation(-90.0f, CaptureActor->CapturedMapYaw, 0.0f);
	const FVector QuarterRightLocation = CaptureActor->CapturedMapCenter
		+ CapturedRotation.RotateVector(FVector::RightVector) * CaptureActor->CapturedMapOrthoWidth * 0.25f;
	FVector2D QuarterRightUV;
	CaptureActor->WorldToMapUV(QuarterRightLocation, QuarterRightUV);
	TestTrue(TEXT("Camera-right world movement maps to positive U"), QuarterRightUV.Equals(FVector2D(0.75f, 0.5f), KINDA_SMALL_NUMBER));

	const FVector QuarterUpLocation = CaptureActor->CapturedMapCenter
		+ CapturedRotation.RotateVector(FVector::UpVector) * CaptureActor->CapturedMapOrthoWidth * 0.25f;
	FVector2D QuarterUpUV;
	CaptureActor->WorldToMapUV(QuarterUpLocation, QuarterUpUV);
	TestTrue(TEXT("Camera-up world movement maps to negative texture V"), QuarterUpUV.Equals(FVector2D(0.5f, 0.25f), KINDA_SMALL_NUMBER));

	UMaterial* StaticMapMaterial = LoadObject<UMaterial>(
		nullptr,
		TEXT("/Game/UI/HUD/Minimap/Materials/M_UI_Minimap_StaticMap.M_UI_Minimap_StaticMap"));
	TestNotNull(TEXT("Static minimap UI material asset exists"), StaticMapMaterial);
	if (StaticMapMaterial)
	{
		TestEqual(TEXT("Static minimap material uses the UI domain"), StaticMapMaterial->MaterialDomain, MD_UI);
		TestEqual(TEXT("Static minimap material supports circular opacity"), StaticMapMaterial->BlendMode, BLEND_Translucent);
	}

	// Validate the production HUD contract so the subsystem always has a visible Image to receive the render target.
	UClass* HUDWidgetClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/HUD/WBP_PlayerHUDWidget.WBP_PlayerHUDWidget_C"));
	UWidgetBlueprintGeneratedClass* HUDGeneratedClass = Cast<UWidgetBlueprintGeneratedClass>(HUDWidgetClass);
	UWidgetTree* HUDWidgetTree = HUDGeneratedClass ? HUDGeneratedClass->GetWidgetTreeArchetype() : nullptr;
	UImage* MinimapBackgroundImage = ResolveProductionMinimapMapImage(HUDWidgetTree);
	TestNotNull(TEXT("Production HUD contains a minimap map Image"), MinimapBackgroundImage);
	if (MinimapBackgroundImage)
	{
		TestTrue(
			TEXT("Production minimap background is visible"),
			MinimapBackgroundImage->GetVisibility() != ESlateVisibility::Collapsed
				&& MinimapBackgroundImage->GetVisibility() != ESlateVisibility::Hidden);
		TestTrue(
			TEXT("Production minimap background supports the runtime marker layer"),
			Cast<UCanvasPanel>(MinimapBackgroundImage->GetParent()) != nullptr
				|| Cast<UOverlay>(MinimapBackgroundImage->GetParent()) != nullptr);
	}

	const FVector GroundCenter(400.0f, 500.0f, 25.0f);
	CaptureActor->ApplyTopDownTransform(GroundCenter, 2200.0f, 0.0f);
	const FVector FirstCaptureLocation = CaptureActor->GetActorLocation();
	CaptureActor->ApplyTopDownTransform(GroundCenter, 2200.0f, 0.0f);
	TestEqual(TEXT("Repeated capture uses an absolute location without height accumulation"), CaptureActor->GetActorLocation(), FirstCaptureLocation);

	CaptureActor->CaptureMode = EGPMinimapCaptureMode::FullMap;
	CaptureActor->Tick(1.0f);
	TestEqual(TEXT("Full-map mode does not move on the follow interval"), CaptureActor->GetActorLocation(), FirstCaptureLocation);

	TestWorld->DestroyWorld(false);
	return true;
}

#endif
