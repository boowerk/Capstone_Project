#if WITH_DEV_AUTOMATION_TESTS

#include "Game/RegionEvents/GP_RedRiftRegionEventActor.h"
#include "Game/RegionEvents/GP_RegionEventData.h"
#include "Game/RegionEvents/GP_ShrineRuinsRegionEventActor.h"
#include "Game/RegionEvents/GP_StructureDefenseRegionEventActor.h"
#include "Misc/AutomationTest.h"

namespace GPRegionEventProductionAssetTests
{
	struct FExpectedProductionEvent
	{
		const TCHAR* AssetPath;
		FName EventId;
		UClass* NativeActorClass;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPRegionEventProductionAssetsTest,
	"ProjectEden.Game.RegionEvents.ProductionAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPRegionEventProductionAssetsTest::RunTest(const FString& Parameters)
{
	using namespace GPRegionEventProductionAssetTests;
	const FExpectedProductionEvent ExpectedEvents[] =
	{
		// The guided graduation route owns exactly these three deterministic encounter definitions.
		{ TEXT("/Game/RegionEvents/Runtime/DA_RE_World_RedRift.DA_RE_World_RedRift"),
			TEXT("world_red_rift"), AGP_RedRiftRegionEventActor::StaticClass() },
		{ TEXT("/Game/RegionEvents/Runtime/DA_RE_World_ShrineRuins.DA_RE_World_ShrineRuins"),
			TEXT("world_shrine_ruins"), AGP_ShrineRuinsRegionEventActor::StaticClass() },
		{ TEXT("/Game/RegionEvents/Runtime/DA_RE_World_StructureDefense.DA_RE_World_StructureDefense"),
			TEXT("world_structure_defense"), AGP_StructureDefenseRegionEventActor::StaticClass() }
	};

	for (const FExpectedProductionEvent& Expected : ExpectedEvents)
	{
		UGP_RegionEventData* EventData = LoadObject<UGP_RegionEventData>(nullptr, Expected.AssetPath);
		TestNotNull(FString::Printf(TEXT("Production event loads: %s"), Expected.AssetPath), EventData);
		if (!IsValid(EventData))
		{
			continue;
		}

		// Production definitions must never expose the duplicated smoke-test identity or Blueprint class in play.
		TestEqual(TEXT("Production event has a stable world id"), EventData->EventId, Expected.EventId);
		TestEqual(TEXT("Production event uses its native runtime class"), EventData->EventActorClass.Get(), Expected.NativeActorClass);
		TestFalse(TEXT("Production label does not expose test content"),
			EventData->DisplayName.ToString().Contains(TEXT("TEST"), ESearchCase::IgnoreCase));
		// Scripted encounters must remain visible before activation without mutating authored biome presentation.
		TestTrue(TEXT("Production events are discoverable before activation"), EventData->bActivateWhenPlayerApproaches);
		TestFalse(TEXT("Production events never overwrite authored biome states"), EventData->bApplyActiveRegionState);
		TestFalse(TEXT("Production completion never overwrites authored biome states"), EventData->bApplyCompletedRegionState);
	}

	return !HasAnyErrors();
}

#endif
