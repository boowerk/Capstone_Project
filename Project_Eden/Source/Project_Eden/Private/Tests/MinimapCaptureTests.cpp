#if WITH_DEV_AUTOMATION_TESTS

#include "Components/SceneCaptureComponent2D.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "UI/GP_MinimapCaptureActor.h"

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
		TestEqual(TEXT("Minimap captures flat base color"), SceneCapture->CaptureSource, ESceneCaptureSource::SCS_BaseColor);
		TestFalse(TEXT("Minimap capture excludes lighting"), SceneCapture->ShowFlags.Lighting);
		TestFalse(TEXT("Minimap capture excludes shadows"), SceneCapture->ShowFlags.DynamicShadows);
		TestFalse(TEXT("Minimap capture excludes particles"), SceneCapture->ShowFlags.Particles);
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
