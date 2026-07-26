#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Game/WorldLayout/GP_VillageLayoutDirector.h"
#include "HAL/IConsoleManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDebugPresentationDefaultsTest,
	"ProjectEden.Debug.PresentationDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDebugPresentationDefaultsTest::RunTest(const FString& Parameters)
{
	if (!TestNotNull(TEXT("The engine config cache is available"), GConfig))
	{
		return false;
	}

	bool bOnScreenMessagesEnabled = true;
	const bool bFoundMessageSetting = GConfig->GetBool(
		TEXT("/Script/Engine.Engine"),
		TEXT("bEnableOnScreenDebugMessages"),
		bOnScreenMessagesEnabled,
		GEngineIni);
	TestTrue(TEXT("DefaultEngine.ini explicitly configures on-screen debug messages"), bFoundMessageSetting);
	TestFalse(TEXT("On-screen debug messages are disabled by default"), bOnScreenMessagesEnabled);

	if (TestNotNull(TEXT("The engine singleton is available"), GEngine))
	{
		TestFalse(
			TEXT("The runtime on-screen debug message gate is disabled"),
			GEngine->bEnableOnScreenDebugMessages);
		TestFalse(
			TEXT("The runtime on-screen debug message display is disabled"),
			GEngine->bEnableOnScreenDebugMessagesDisplay);
	}

	const AGP_VillageLayoutDirector* VillageDefaults =
		GetDefault<AGP_VillageLayoutDirector>();
	TestFalse(
		TEXT("Village debug drawing is disabled on the native CDO"),
		VillageDefaults->IsDebugDrawingRequested());

	IConsoleVariable* VillageDebugDrawCVar =
		IConsoleManager::Get().FindConsoleVariable(TEXT("gp.Village.DebugDraw"));
	IConsoleVariable* SkillDebugDrawCVar =
		IConsoleManager::Get().FindConsoleVariable(TEXT("g.DrawSkillDebug"));
	if (TestNotNull(TEXT("Village debug draw opt-in CVar is registered"), VillageDebugDrawCVar))
	{
		TestEqual(TEXT("Village debug draw CVar defaults to off"), VillageDebugDrawCVar->GetInt(), 0);
	}
	if (TestNotNull(TEXT("Skill debug draw opt-in CVar is registered"), SkillDebugDrawCVar))
	{
		TestEqual(TEXT("Skill debug draw CVar defaults to off"), SkillDebugDrawCVar->GetInt(), 0);
	}
	return true;
}

#endif
