#if WITH_DEV_AUTOMATION_TESTS

#include "Actors/GP_LevelBuildAnimator.h"
#include "Engine/World.h"
#include "Game/GP_EnemySpawnVolume.h"
#include "Game/GP_GameMode.h"
#include "Game/GP_RunPortal.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPColosseumArrivalPolicyTest,
	"ProjectEden.Game.Colosseum.ArrivalPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPColosseumArrivalPolicyTest::RunTest(const FString& Parameters)
{
	TestTrue(
		TEXT("Ordinary staged zones still count physical presence"),
		AGP_GameMode::CanCountStagedZonePresence(
			EGPZoneStage::Center,
			/*bHasMatchingPortalArrival=*/false));
	TestFalse(
		TEXT("Colosseum rejects direct or debug-teleport entry"),
		AGP_GameMode::CanCountStagedZonePresence(
			EGPZoneStage::Colosseum,
			/*bHasMatchingPortalArrival=*/false));
	TestTrue(
		TEXT("Colosseum counts a matching authority portal arrival"),
		AGP_GameMode::CanCountStagedZonePresence(
			EGPZoneStage::Colosseum,
			/*bHasMatchingPortalArrival=*/true));

	TestFalse(
		TEXT("Colosseum does not start before the active party arrives"),
		AGP_GameMode::ShouldStartColosseumIntro(
			/*bAllActivePlayersPresent=*/false,
			/*bIntroStarted=*/false,
			/*bIntroCompleted=*/false));
	TestTrue(
		TEXT("Colosseum starts once after the active party arrives"),
		AGP_GameMode::ShouldStartColosseumIntro(
			/*bAllActivePlayersPresent=*/true,
			/*bIntroStarted=*/false,
			/*bIntroCompleted=*/false));
	TestFalse(
		TEXT("Colosseum intro cannot restart while playing"),
		AGP_GameMode::ShouldStartColosseumIntro(
			/*bAllActivePlayersPresent=*/true,
			/*bIntroStarted=*/true,
			/*bIntroCompleted=*/false));
	TestFalse(
		TEXT("Colosseum intro cannot restart after completion"),
		AGP_GameMode::ShouldStartColosseumIntro(
			/*bAllActivePlayersPresent=*/true,
			/*bIntroStarted=*/true,
			/*bIntroCompleted=*/true));
	TestTrue(
		TEXT("Center still checks the live party before encounter start"),
		AGP_GameMode::RequiresFullPartyAtEncounterStart(
			EGPZoneStage::Center));
	TestFalse(
		TEXT("Colosseum does not re-gate on a reconnect after intro admission"),
		AGP_GameMode::RequiresFullPartyAtEncounterStart(
			EGPZoneStage::Colosseum));
	TestFalse(
		TEXT("A joiner is not relocated before Colosseum admission"),
		AGP_GameMode::ShouldRelocateJoiningPlayerToZone(
			EGPZoneStage::Colosseum,
			/*bIntroStarted=*/false,
			/*bIntroCompleted=*/false,
			/*bEncounterStarted=*/false,
			/*bZoneCompleted=*/false));
	TestTrue(
		TEXT("A joiner follows the party while the Colosseum intro is playing"),
		AGP_GameMode::ShouldRelocateJoiningPlayerToZone(
			EGPZoneStage::Colosseum,
			/*bIntroStarted=*/true,
			/*bIntroCompleted=*/false,
			/*bEncounterStarted=*/false,
			/*bZoneCompleted=*/false));
	TestTrue(
		TEXT("A joiner follows the party after the boss encounter starts"),
		AGP_GameMode::ShouldRelocateJoiningPlayerToZone(
			EGPZoneStage::Colosseum,
			/*bIntroStarted=*/true,
			/*bIntroCompleted=*/true,
			/*bEncounterStarted=*/true,
			/*bZoneCompleted=*/false));
	TestFalse(
		TEXT("A joiner is not relocated into a completed Colosseum"),
		AGP_GameMode::ShouldRelocateJoiningPlayerToZone(
			EGPZoneStage::Colosseum,
			/*bIntroStarted=*/true,
			/*bIntroCompleted=*/true,
			/*bEncounterStarted=*/true,
			/*bZoneCompleted=*/true));

	TestFalse(
		TEXT("Stale portal entries cannot replace a missing active player"),
		AGP_RunPortal::IsActivePartyComplete(
			/*ActivePlayerCount=*/2,
			/*EnteredActivePlayerCount=*/1));
	TestTrue(
		TEXT("Portal closes after every current active player entered"),
		AGP_RunPortal::IsActivePartyComplete(
			/*ActivePlayerCount=*/2,
			/*EnteredActivePlayerCount=*/2));
	TestFalse(
		TEXT("An empty active party cannot activate the portal"),
		AGP_RunPortal::IsActivePartyComplete(
			/*ActivePlayerCount=*/0,
			/*EnteredActivePlayerCount=*/0));

	const FGPZoneRuntimeState RuntimeDefaults;
	TestTrue(
		TEXT("Zone runtime begins without portal arrival credit"),
		RuntimeDefaults.PortalArrivals.IsEmpty());
	TestFalse(
		TEXT("Zone runtime begins before its entrance presentation"),
		RuntimeDefaults.bIntroStarted);
	TestFalse(
		TEXT("Zone runtime begins with an incomplete entrance presentation"),
		RuntimeDefaults.bIntroCompleted);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPColosseumAnimatorReplicationContractTest,
	"ProjectEden.Game.Colosseum.AnimatorReplicationContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPColosseumAnimatorReplicationContractTest::RunTest(
	const FString& Parameters)
{
	const AGP_LevelBuildAnimator* NativeDefaults =
		GetDefault<AGP_LevelBuildAnimator>();
	TestTrue(
		TEXT("Level build animator replicates its playback snapshot"),
		NativeDefaults->GetIsReplicated());
	TestTrue(
		TEXT("Level build animator stays relevant for late arrivals"),
		NativeDefaults->bAlwaysRelevant);

	const FProperty* SnapshotProperty =
		AGP_LevelBuildAnimator::StaticClass()->FindPropertyByName(
			TEXT("PlaybackSnapshot"));
	TestNotNull(
		TEXT("Level build animator exposes one atomic playback snapshot"),
		SnapshotProperty);
	TestTrue(
		TEXT("Playback snapshot is replicated"),
		SnapshotProperty
			&& SnapshotProperty->HasAnyPropertyFlags(CPF_Net));
	TestTrue(
		TEXT("Playback snapshot uses RepNotify"),
		SnapshotProperty
			&& SnapshotProperty->HasAnyPropertyFlags(CPF_RepNotify));
	TestEqual(
		TEXT("Playback snapshot uses the expected RepNotify"),
		SnapshotProperty ? SnapshotProperty->RepNotifyFunc : NAME_None,
		FName(TEXT("OnRep_PlaybackSnapshot")));
	TestNotNull(
		TEXT("Playback snapshot RepNotify exists"),
		AGP_LevelBuildAnimator::StaticClass()->FindFunctionByName(
			TEXT("OnRep_PlaybackSnapshot")));

	const FBoolProperty* AutoStartProperty = FindFProperty<FBoolProperty>(
		AGP_LevelBuildAnimator::StaticClass(),
		TEXT("bAutoStartOnBeginPlay"));
	const FBoolProperty* PortalWaitProperty = FindFProperty<FBoolProperty>(
		AGP_LevelBuildAnimator::StaticClass(),
		TEXT("bWaitForColosseumPortalArrival"));
	TestTrue(
		TEXT("BeginPlay auto-start is disabled by default"),
		AutoStartProperty
			&& !AutoStartProperty->GetPropertyValue_InContainer(NativeDefaults));
	TestTrue(
		TEXT("Map animator waits for the Colosseum portal by default"),
		PortalWaitProperty
			&& PortalWaitProperty->GetPropertyValue_InContainer(NativeDefaults));
	TestTrue(
		TEXT("A single map animator defaults to automatic Colosseum association"),
		NativeDefaults->GetColosseumZoneId().IsNone());

	const UClass* MapAnimatorClass = LoadClass<AGP_LevelBuildAnimator>(
		nullptr,
		TEXT("/Game/Meshes/PLAZA_DE_TOROS/StructureModel/"
			"BP_GP_LevelBuildAnimator.BP_GP_LevelBuildAnimator_C"));
	const AGP_LevelBuildAnimator* MapAnimatorDefaults =
		MapAnimatorClass
			? Cast<AGP_LevelBuildAnimator>(
				MapAnimatorClass->GetDefaultObject())
			: nullptr;
	TestNotNull(
		TEXT("Loads the actual map-placed Colosseum animator class"),
		MapAnimatorDefaults);
	TestTrue(
		TEXT("Actual Colosseum animator inherits portal-gated playback"),
		MapAnimatorDefaults
			&& PortalWaitProperty
			&& PortalWaitProperty->GetPropertyValue_InContainer(
				MapAnimatorDefaults));
	TestTrue(
		TEXT("Actual Colosseum animator CDO is replicated"),
		MapAnimatorDefaults
			&& MapAnimatorDefaults->GetIsReplicated());
	TestTrue(
		TEXT("Actual Colosseum animator CDO is always relevant"),
		MapAnimatorDefaults
			&& MapAnimatorDefaults->bAlwaysRelevant);

	const FGPLevelBuildPlaybackSnapshot DefaultSnapshot;
	TestEqual(
		TEXT("Playback snapshot starts at revision zero"),
		DefaultSnapshot.Revision,
		0);
	TestEqual(
		TEXT("Playback snapshot starts waiting"),
		DefaultSnapshot.Phase,
		EGPLevelBuildPlaybackPhase::Waiting);
	TestEqual(
		TEXT("Playback snapshot starts without a duration"),
		DefaultSnapshot.Duration,
		0.0f);

	const FProperty* PortalTargetZoneProperty =
		AGP_RunPortal::StaticClass()->FindPropertyByName(TEXT("TargetZone"));
	TestNotNull(
		TEXT("Run portal keeps its authoritative destination zone"),
		PortalTargetZoneProperty);
	TestTrue(
		TEXT("Portal destination zone is transient server runtime state"),
		PortalTargetZoneProperty
			&& PortalTargetZoneProperty->HasAnyPropertyFlags(CPF_Transient));
	TestFalse(
		TEXT("Portal CDO begins without an unsafe default destination"),
		GetDefault<AGP_RunPortal>()->IsDestinationConfigured());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPColosseumAnimatorAuthorityStartTest,
	"ProjectEden.Game.Colosseum.AnimatorAuthorityStart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPColosseumAnimatorAuthorityStartTest::RunTest(
	const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Created Colosseum animator test world"), TestWorld))
	{
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* TaggedPiece = TestWorld->SpawnActor<AActor>(
		FVector(200.0f, 0.0f, 500.0f),
		FRotator::ZeroRotator,
		SpawnParameters);
	AGP_LevelBuildAnimator* Animator =
		TestWorld->SpawnActor<AGP_LevelBuildAnimator>(
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			SpawnParameters);
	if (!TestNotNull(TEXT("Spawned tagged Colosseum piece"), TaggedPiece)
		|| !TestNotNull(TEXT("Spawned Colosseum animator"), Animator))
	{
		TestWorld->DestroyWorld(false);
		return false;
	}

	TaggedPiece->Tags.Add(TEXT("PLAZA_DE_TOROS"));
	Animator->RebuildPieceList();
	TestTrue(
		TEXT("Animator discovers tagged Colosseum pieces"),
		Animator->GetTotalBuildTime() > 0.0f);
	TestTrue(
		TEXT("Authority starts the Colosseum presentation"),
		Animator->StartBuildAuthoritative());
	TestEqual(
		TEXT("Authority publishes a playing snapshot"),
		Animator->GetPlaybackPhase(),
		EGPLevelBuildPlaybackPhase::Playing);
	TestEqual(
		TEXT("First playback uses revision one"),
		Animator->GetPlaybackSnapshot().Revision,
		1);
	TestTrue(
		TEXT("Published playback duration matches the authored build"),
		FMath::IsNearlyEqual(
			Animator->GetPlaybackSnapshot().Duration,
			Animator->GetTotalBuildTime()));
	TestFalse(
		TEXT("Repeated authority start cannot restart the same presentation"),
		Animator->StartBuildAuthoritative());

	int32 FinishBroadcastCount = 0;
	Animator->OnBuildFinishedNative.AddLambda(
		[&FinishBroadcastCount](AGP_LevelBuildAnimator*)
		{
			++FinishBroadcastCount;
		});
	Animator->FinishBuild();
	TestEqual(
		TEXT("Authority publishes a finished snapshot"),
		Animator->GetPlaybackPhase(),
		EGPLevelBuildPlaybackPhase::Finished);
	TestEqual(
		TEXT("Authority completion broadcasts exactly once"),
		FinishBroadcastCount,
		1);
	Animator->FinishBuild();
	TestEqual(
		TEXT("Repeated completion cannot re-open the boss gate"),
		FinishBroadcastCount,
		1);

	TestWorld->DestroyWorld(false);
	return true;
}

#endif
