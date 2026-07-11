#if WITH_DEV_AUTOMATION_TESTS

#include "Game/Corruption/GP_CorruptionTestZoneActor.h"

#include "Components/BoxComponent.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPCorruptionTestZoneContractTest,
	"ProjectEden.Game.Corruption.TestZoneContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPCorruptionTestZoneContractTest::RunTest(const FString& Parameters)
{
	const AGP_CorruptionTestZoneActor* TestZone = GetDefault<AGP_CorruptionTestZoneActor>();
	TestNotNull(TEXT("Native corruption test station CDO exists"), TestZone);
	if (!TestZone)
	{
		return false;
	}

	TestNotNull(TEXT("Test station owns a walk-in overlap box"), TestZone->GetTriggerBox());
	TestEqual(TEXT("Default test station starts from clean corruption"), TestZone->GetTestCorruption(), 0.0f);
	if (const UBoxComponent* TriggerBox = TestZone->GetTriggerBox())
	{
		TestTrue(TEXT("Test station detects pawn overlaps"), TriggerBox->GetGenerateOverlapEvents());
	}

	return true;
}

#endif
