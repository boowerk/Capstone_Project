#if WITH_DEV_AUTOMATION_TESTS

#include "Actors/GP_BossGroundHandActor.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBossGroundHandUsesRightHandMeshTest,
	"ProjectEden.AI.Boss.GroundHands.UsesRightHandMesh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBossGroundHandUsesRightHandMeshTest::RunTest(const FString& Parameters)
{
	// Protect the native GAS pattern from silently returning to the old primitive placeholder presentation.
	const AGP_BossGroundHandActor* GroundHand = GetDefault<AGP_BossGroundHandActor>();
	const USkeletalMeshComponent* HandMesh = IsValid(GroundHand)
		? GroundHand->FindComponentByClass<USkeletalMeshComponent>()
		: nullptr;
	const USkeletalMesh* MeshAsset = IsValid(HandMesh) ? HandMesh->GetSkeletalMeshAsset() : nullptr;

	TestNotNull(TEXT("Ground hand owns a skeletal presentation component"), HandMesh);
	TestNotNull(TEXT("Ground hand loads the imported right-hand asset"), MeshAsset);
	if (IsValid(MeshAsset))
	{
		TestEqual(
			TEXT("Ground hand uses SK_RightHand from PLAZA_DE_TOROS"),
			MeshAsset->GetPathName(),
			FString(TEXT("/Game/Meshes/PLAZA_DE_TOROS/ActorMesh/SK_RightHand.SK_RightHand")));
	}

	return true;
}

#endif
