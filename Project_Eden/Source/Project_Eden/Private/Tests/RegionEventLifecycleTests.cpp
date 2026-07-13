#if WITH_DEV_AUTOMATION_TESTS

#include "Characters/GP_EnemyCharacter.h"
#include "Components/DecalComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "Game/RegionEvents/GP_RegionEventActor.h"
#include "Game/RegionEvents/GP_RegionEventData.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

namespace GPRegionEventLifecycleTests
{
	bool InjectTrackedEnemy(
		FAutomationTestBase& Test,
		AGP_RegionEventActor& EventActor,
		AGP_EnemyCharacter& Enemy)
	{
		// Reflection keeps the production tracking array private while still exercising both public lifecycle exits.
		const FArrayProperty* EnemyArrayProperty =
			FindFProperty<FArrayProperty>(EventActor.GetClass(), TEXT("SpawnedEventEnemies"));
		if (!Test.TestNotNull(TEXT("Event actor exposes its tracked-enemy storage"), EnemyArrayProperty))
		{
			return false;
		}

		FScriptArrayHelper EnemyArray(
			EnemyArrayProperty,
			EnemyArrayProperty->ContainerPtrToValuePtr<void>(&EventActor));
		const int32 NewIndex = EnemyArray.AddValue();
		const FObjectPropertyBase* EnemyObjectProperty = CastField<FObjectPropertyBase>(EnemyArrayProperty->Inner);
		if (!Test.TestNotNull(TEXT("Tracked-enemy storage contains object references"), EnemyObjectProperty))
		{
			return false;
		}

		EnemyObjectProperty->SetObjectPropertyValue(EnemyArray.GetRawPtr(NewIndex), &Enemy);
		return true;
	}
}

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
		// Open-world objectives own their spawned combatants and must not leak AI after completion or expiration.
		TestTrue(TEXT("Finished events retire their remaining enemies"), ActorDefaults->ShouldRetireSpawnedEnemiesOnEnd());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRegionEventEnemyRetirementTest,
	"ProjectEden.Game.RegionEvents.EnemyRetirement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRegionEventEnemyRetirementTest::RunTest(const FString& Parameters)
{
	using namespace GPRegionEventLifecycleTests;
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Created transient region-event world"), TestWorld))
	{
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	UGP_RegionEventData* EventData = NewObject<UGP_RegionEventData>();
	EventData->DurationSeconds = -1.0f;

	const auto VerifyExitRetiresEnemy = [this, TestWorld, &SpawnParameters, EventData](bool bComplete)
	{
		AGP_RegionEventActor* EventActor = TestWorld->SpawnActor<AGP_RegionEventActor>(
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			SpawnParameters);
		AGP_EnemyCharacter* Enemy = TestWorld->SpawnActor<AGP_EnemyCharacter>(
			FVector(200.0f, 0.0f, 0.0f),
			FRotator::ZeroRotator,
			SpawnParameters);
		if (!TestNotNull(TEXT("Spawned lifecycle event actor"), EventActor)
			|| !TestNotNull(TEXT("Spawned lifecycle enemy"), Enemy))
		{
			return false;
		}

		EventActor->InitializeRegionEvent(0, EventData);
		if (!InjectTrackedEnemy(*this, *EventActor, *Enemy))
		{
			return false;
		}

		TestEqual(TEXT("Event tracks one live enemy before ending"), EventActor->GetAliveSpawnedEnemyCount(), 1);
		if (bComplete)
		{
			EventActor->CompleteRegionEvent();
		}
		else
		{
			EventActor->ExpireRegionEvent();
		}

		TestTrue(bComplete
			? TEXT("Completion retires its remaining enemy")
			: TEXT("Expiration retires its remaining enemy"), Enemy->IsDead());
		TestEqual(TEXT("Ended event reports no live tracked enemies"), EventActor->GetAliveSpawnedEnemyCount(), 0);
		return true;
	};

	// Success and timeout must share the same ownership invariant so neither path leaks encounter AI.
	VerifyExitRetiresEnemy(/*bComplete=*/true);
	VerifyExitRetiresEnemy(/*bComplete=*/false);
	TestWorld->DestroyWorld(false);
	return !HasAnyErrors();
}

#endif
