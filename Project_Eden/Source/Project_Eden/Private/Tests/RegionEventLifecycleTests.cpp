#if WITH_DEV_AUTOMATION_TESTS

#include "Components/DecalComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "Game/RegionEvents/GP_RegionEventActor.h"
#include "Game/RegionEvents/GP_RegionEventData.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRegionEventLifecycleDefaultsTest,
	"ProjectEden.Game.RegionEvents.LifecycleDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRegionEventLifecycleDefaultsTest::RunTest(const FString& Parameters)
{
	const UGP_RegionEventData* EventDefaults = GetDefault<UGP_RegionEventData>();
	TestNotNull(TEXT("Region event DataAsset defaults exist"), EventDefaults);
	if (EventDefaults)
	{
		// These neutral defaults are a regression guard for every event asset authored before discovery/corruption flow existed.
		TestEqual(TEXT("Legacy events accept uncorrupted regions"), EventDefaults->MinimumCorruptionNormalized, 0.0f);
		TestEqual(TEXT("Legacy events accept fully corrupted regions"), EventDefaults->MaximumCorruptionNormalized, 1.0f);
		TestEqual(TEXT("Legacy success does not mutate corruption"), EventDefaults->CompletionCorruptionReduction, 0.0f);
		TestEqual(TEXT("Legacy failure does not mutate corruption"), EventDefaults->ExpirationCorruptionIncrease, 0.0f);
		TestEqual(TEXT("Legacy events add no cooldown"), EventDefaults->RegionCooldownSeconds, 0.0f);
		TestFalse(TEXT("Legacy events do not block zone completion"), EventDefaults->bBlocksZoneCompletion);
		TestFalse(TEXT("Legacy events activate immediately"), EventDefaults->bActivateWhenPlayerApproaches);
		TestEqual(TEXT("Discovery radius has a playable default"), EventDefaults->ApproachActivationRadius, 700.0f);
		TestEqual(TEXT("Discovery waits indefinitely unless authored otherwise"), EventDefaults->DormantWaitTimeoutSeconds, -1.0f);
	}

	const AGP_RegionEventActor* ActorDefaults = GetDefault<AGP_RegionEventActor>();
	TestNotNull(TEXT("Native event actor defaults exist"), ActorDefaults);
	if (ActorDefaults)
	{
		// A code-only event must still be discoverable without any Blueprint construction script.
		TestNotNull(TEXT("Native event has a ground decal"), ActorDefaults->FindComponentByClass<UDecalComponent>());
		TestNotNull(TEXT("Native event has a marker light"), ActorDefaults->FindComponentByClass<UPointLightComponent>());
		TestNotNull(TEXT("Native event has a text marker"), ActorDefaults->FindComponentByClass<UTextRenderComponent>());
		TestFalse(TEXT("Uninitialized event does not block progression"), ActorDefaults->IsBlockingZoneCompletion());
		TestEqual(TEXT("Uninitialized event tracks no enemies"), ActorDefaults->GetAliveSpawnedEnemyCount(), 0);
	}

	return true;
}

#endif
