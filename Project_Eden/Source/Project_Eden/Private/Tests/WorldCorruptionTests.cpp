#if WITH_DEV_AUTOMATION_TESTS

#include "Game/Corruption/GP_WorldCorruptionComponent.h"

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPWorldCorruptionStateTest,
	"ProjectEden.Game.Corruption.State",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPWorldCorruptionStateTest::RunTest(const FString& Parameters)
{
	UGP_WorldCorruptionComponent* Corruption = NewObject<UGP_WorldCorruptionComponent>();
	TestNotNull(TEXT("Corruption component can be created without a world for deterministic tests"), Corruption);
	if (!Corruption)
	{
		return false;
	}

	Corruption->InitializeCorruption(
		/*RegionCount=*/4,
		/*InitialCorruption=*/20.0f,
		/*InMaximumCorruption=*/100.0f,
		/*InPassiveIncreasePerMinute=*/0.0f,
		/*InPassiveTickInterval=*/5.0f,
		/*bEnablePassiveIncrease=*/false);

	TestEqual(TEXT("Initialization creates one value per region"), Corruption->GetRegionCount(), 4);
	TestEqual(TEXT("Initial world corruption matches the regional average"), Corruption->GetWorldCorruption(), 20.0f);

	Corruption->ReduceRegionCorruption(/*RegionId=*/2, /*ReductionAmount=*/20.0f);
	TestEqual(TEXT("Boss-style regional reduction affects only its region"), Corruption->GetRegionCorruption(2), 0.0f);
	TestEqual(TEXT("World corruption is recalculated as the regional average"), Corruption->GetWorldCorruption(), 15.0f);

	Corruption->AddWorldCorruption(10.0f);
	TestEqual(TEXT("World progression advances unaffected regions"), Corruption->GetRegionCorruption(0), 30.0f);
	TestEqual(TEXT("World progression also advances the cleansed region"), Corruption->GetRegionCorruption(2), 10.0f);
	TestEqual(TEXT("World average remains consistent after a global increase"), Corruption->GetWorldCorruption(), 25.0f);

	Corruption->SetRegionCorruption(/*RegionId=*/0, /*NewCorruption=*/1000.0f);
	TestEqual(TEXT("Regional values clamp to the configured maximum"), Corruption->GetRegionCorruption(0), 100.0f);
	TestEqual(TEXT("Normalized values remain in the zero-to-one presentation range"), Corruption->GetRegionCorruptionNormalized(0), 1.0f);

	Corruption->SetPassiveIncreasePaused(true);
	TestTrue(TEXT("Test environments can pause passive growth without resetting values"), Corruption->IsPassiveIncreasePaused());
	Corruption->SetPassiveIncreasePaused(false);
	TestFalse(TEXT("Passive growth can be resumed after a test"), Corruption->IsPassiveIncreasePaused());

	return true;
}

#endif
