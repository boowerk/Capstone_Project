#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Characters/GP_PlayerCharacter.h"
#include "NavigationData.h"
#include "NavigationInvokerComponent.h"
#include "NavigationSystem.h"
#include "NavMesh/RecastNavMesh.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPPlayerNavigationInvokerContractTest,
	"ProjectEden.AI.Navigation.PlayerInvokerContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPPlayerNavigationInvokerContractTest::RunTest(const FString& Parameters)
{
	const AGP_PlayerCharacter* PlayerDefaults = GetDefault<AGP_PlayerCharacter>();
	TestNotNull(TEXT("Player class defaults are available"), PlayerDefaults);
	if (!PlayerDefaults)
	{
		return false;
	}

	const UNavigationInvokerComponent* NavigationInvoker =
		PlayerDefaults->FindComponentByClass<UNavigationInvokerComponent>();
	TestNotNull(TEXT("Every player owns a navigation invoker"), NavigationInvoker);
	if (!NavigationInvoker)
	{
		return false;
	}

	// These defaults cover nearby combat while keeping the generated tile set bounded on the large landscape.
	TestEqual(TEXT("Default tile generation radius is 5000 cm"), NavigationInvoker->GetGenerationRadius(), 5000.0f);
	TestEqual(TEXT("Default tile removal radius is 7000 cm"), NavigationInvoker->GetRemovalRadius(), 7000.0f);
	TestTrue(TEXT("Removal radius stays wider than generation radius"),
		NavigationInvoker->GetRemovalRadius() > NavigationInvoker->GetGenerationRadius());

	const UNavigationSystemV1* NavigationDefaults = GetDefault<UNavigationSystemV1>();
	TestNotNull(TEXT("Navigation system defaults are available"), NavigationDefaults);
	if (NavigationDefaults)
	{
		// This guards against accidentally restoring an expensive whole-map navigation build.
		TestTrue(TEXT("Navigation generation remains restricted to invokers"),
			NavigationDefaults->IsActiveTilesGenerationEnabled());
	}

	const ARecastNavMesh* RecastDefaults = GetDefault<ARecastNavMesh>();
	TestNotNull(TEXT("Recast defaults are available"), RecastDefaults);
	if (RecastDefaults)
	{
		// Dynamic generation is required for the active tile window to follow moving players.
		TestEqual(TEXT("Recast runtime generation remains dynamic"),
			RecastDefaults->GetRuntimeGenerationMode(), ERuntimeGenerationType::Dynamic);
	}

	return !HasAnyErrors();
}

#endif
