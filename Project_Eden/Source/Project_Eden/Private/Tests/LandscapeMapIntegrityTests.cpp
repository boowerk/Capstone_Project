#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/ActorComponent.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPLandscapeMapIntegrityTest,
	"ProjectEden.Game.LandscapeMap.Integrity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPLandscapeMapIntegrityTest::RunTest(const FString& Parameters)
{
	constexpr TCHAR MapPackagePath[] = TEXT("/Game/Maps/MainMap/L_LandscapeMap");
	const FName TestEnvironmentTag(TEXT("GP.TestEnvironment"));
	const FName LandscapeClassName(TEXT("Landscape"));
	const FName LandscapeProxyClassName(TEXT("LandscapeStreamingProxy"));
	const FName LandscapeComponentClassName(TEXT("LandscapeComponent"));
	const FName LandscapeCollisionClassName(TEXT("LandscapeHeightfieldCollisionComponent"));
	UPackage* MapPackage = LoadPackage(nullptr, MapPackagePath, LOAD_None);
	TestNotNull(TEXT("L_LandscapeMap package loads"), MapPackage);

	UWorld* World = MapPackage ? UWorld::FindWorldInPackage(MapPackage) : nullptr;
	TestNotNull(TEXT("L_LandscapeMap contains a world"), World);
	if (!World || !World->PersistentLevel)
	{
		return false;
	}

	int32 LandscapeActorCount = 0;
	int32 LandscapeComponentCount = 0;
	int32 LandscapeCollisionCount = 0;
	int32 TestEnvironmentActorCount = 0;
	for (AActor* Actor : World->PersistentLevel->Actors)
	{
		if (!IsValid(Actor))
		{
			continue;
		}

		if (Actor->Tags.Contains(TestEnvironmentTag))
		{
			++TestEnvironmentActorCount;
		}

		const FName ActorClassName = Actor->GetClass()->GetFName();
		if (ActorClassName == LandscapeClassName || ActorClassName == LandscapeProxyClassName)
		{
			++LandscapeActorCount;
		}

		// Class-name checks avoid adding a runtime Landscape module dependency solely for an editor integrity test.
		TInlineComponentArray<UActorComponent*> Components(Actor);
		for (const UActorComponent* Component : Components)
		{
			if (!IsValid(Component))
			{
				continue;
			}

			const FName ComponentClassName = Component->GetClass()->GetFName();
			if (ComponentClassName == LandscapeComponentClassName)
			{
				++LandscapeComponentCount;
			}
			else if (ComponentClassName == LandscapeCollisionClassName)
			{
				++LandscapeCollisionCount;
			}
		}
	}

	// These assertions catch the prior regression where a commandlet save retained the map but stripped sculpt data.
	TestTrue(TEXT("Sculpted Landscape actor remains present"), LandscapeActorCount > 0);
	TestTrue(TEXT("Sculpted Landscape components remain present"), LandscapeComponentCount > 0);
	TestTrue(TEXT("Landscape collision components remain present"), LandscapeCollisionCount > 0);
	TestEqual(TEXT("Production landscape contains no generated test-deck actors"), TestEnvironmentActorCount, 0);
	return true;
}

#endif
