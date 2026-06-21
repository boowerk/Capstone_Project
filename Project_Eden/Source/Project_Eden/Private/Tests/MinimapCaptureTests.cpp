#if WITH_DEV_AUTOMATION_TESTS

#include "Components/SceneCaptureComponent2D.h"
#include "Components/Image.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "UI/GP_MinimapCaptureActor.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"

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

	UTextureRenderTarget2D* StableFrontBuffer = CaptureActor->RenderTarget.Get();
	UTextureRenderTarget2D* PendingBackBuffer = CaptureActor->CaptureBackBuffer.Get();
	CaptureActor->RequestCapture();
	TestTrue(TEXT("Minimap capture arms an RHI GPU fence"), CaptureActor->HasCaptureGPUFence());
	TestTrue(TEXT("Minimap marks the GPU capture as pending"), CaptureActor->bHasPendingCapture);

	CaptureActor->CaptureGPUFenceCompletionOverride = false;
	CaptureActor->PromoteCompletedCapture();
	TestTrue(TEXT("Incomplete GPU capture keeps the existing HUD render target"), CaptureActor->RenderTarget.Get() == StableFrontBuffer);
	TestTrue(TEXT("Incomplete GPU capture keeps the same back buffer pending"), CaptureActor->CaptureBackBuffer.Get() == PendingBackBuffer);
	TestTrue(TEXT("Incomplete GPU capture remains pending"), CaptureActor->bHasPendingCapture);

	CaptureActor->CaptureGPUFenceCompletionOverride = true;
	CaptureActor->PromoteCompletedCapture();
	TestTrue(TEXT("Completed GPU capture promotes the finished back buffer"), CaptureActor->RenderTarget.Get() == PendingBackBuffer);
	TestFalse(TEXT("Completed GPU capture clears the pending state"), CaptureActor->bHasPendingCapture);
	CaptureActor->CaptureGPUFenceCompletionOverride.Reset();

	// Validate the production HUD contract so the subsystem always has a visible Image to receive the render target.
	UClass* HUDWidgetClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/HUD/WBP_PlayerHUDWidget.WBP_PlayerHUDWidget_C"));
	UWidgetBlueprintGeneratedClass* HUDGeneratedClass = Cast<UWidgetBlueprintGeneratedClass>(HUDWidgetClass);
	UWidgetTree* HUDWidgetTree = HUDGeneratedClass ? HUDGeneratedClass->GetWidgetTreeArchetype() : nullptr;
	UImage* MinimapBackgroundImage = HUDWidgetTree
		? Cast<UImage>(HUDWidgetTree->FindWidget(TEXT("MinimapBackgroundImage")))
		: nullptr;
	TestNotNull(TEXT("Production HUD contains MinimapBackgroundImage"), MinimapBackgroundImage);
	if (MinimapBackgroundImage)
	{
		TestTrue(
			TEXT("Production minimap background is visible"),
			MinimapBackgroundImage->GetVisibility() != ESlateVisibility::Collapsed
				&& MinimapBackgroundImage->GetVisibility() != ESlateVisibility::Hidden);
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
