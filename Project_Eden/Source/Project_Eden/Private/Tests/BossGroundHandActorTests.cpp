#if WITH_DEV_AUTOMATION_TESTS

#include "Actors/GP_BossGroundHandActor.h"
#include "AbilitySystem/Abilities/Enemy/GP_BossGroundHandsAttack.h"

#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
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
	const UBoxComponent* HandCollision = IsValid(GroundHand)
		? GroundHand->FindComponentByClass<UBoxComponent>()
		: nullptr;
	TArray<UStaticMeshComponent*> LegacyPlaceholderMeshes;
	if (IsValid(GroundHand))
	{
		GroundHand->GetComponents<UStaticMeshComponent>(LegacyPlaceholderMeshes);
	}

	TestNotNull(TEXT("Ground hand owns a skeletal presentation component"), HandMesh);
	TestNotNull(TEXT("Ground hand loads the imported right-hand asset"), MeshAsset);
	TestEqual(TEXT("Ground hand no longer owns primitive placeholder meshes"), LegacyPlaceholderMeshes.Num(), 0);
	if (IsValid(MeshAsset))
	{
		TestEqual(
			TEXT("Ground hand uses SK_RightHand from PLAZA_DE_TOROS"),
			MeshAsset->GetPathName(),
			FString(TEXT("/Game/Meshes/PLAZA_DE_TOROS/ActorMesh/SK_RightHand.SK_RightHand")));
	}

	// Lock the original hit box independently from the presentation mesh so future art swaps cannot change gameplay hits.
	TestNotNull(TEXT("Ground hand keeps its independent box collision"), HandCollision);
	if (IsValid(HandCollision))
	{
		TestEqual(TEXT("Ground hand collision location remains unchanged"), HandCollision->GetRelativeLocation(), FVector(0.0f, 0.0f, 45.0f));
		TestEqual(TEXT("Ground hand collision extent remains unchanged"), HandCollision->GetUnscaledBoxExtent(), FVector(55.0f, 62.0f, 110.0f));
		TestEqual(TEXT("Ground hand collision starts disabled until the rise"), HandCollision->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
		TestEqual(TEXT("Ground hand collision still overlaps pawns"), HandCollision->GetCollisionResponseToChannel(ECC_Pawn), ECR_Overlap);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBossGroundHandUsesVisualBlueprintTest,
	"ProjectEden.AI.Boss.GroundHands.UsesVisualBlueprint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBossGroundHandUsesVisualBlueprintTest::RunTest(const FString& Parameters)
{
	// Keep GAS routed through the designer-tunable BP while native code remains the collision and damage authority.
	const UGP_BossGroundHandsAttack* Ability = GetDefault<UGP_BossGroundHandsAttack>();
	TSubclassOf<AGP_BossGroundHandActor> GroundHandClass;
	if (IsValid(Ability))
	{
		GroundHandClass = Ability->GetGroundHandActorClass();
	}
	const AGP_BossGroundHandActor* VisualDefaults = GroundHandClass
		? GroundHandClass->GetDefaultObject<AGP_BossGroundHandActor>()
		: nullptr;

	TestNotNull(TEXT("Ground Hands ability resolves a presentation actor class"), GroundHandClass.Get());
	if (GroundHandClass)
	{
		TestEqual(
			TEXT("Ground Hands ability uses the Sans visual Blueprint"),
			GroundHandClass->GetPathName(),
			FString(TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_Sans/BP_BossGroundHandActor.BP_BossGroundHandActor_C")));
	}
	TestNotNull(TEXT("Ground Hands visual Blueprint has valid defaults"), VisualDefaults);
	if (IsValid(VisualDefaults))
	{
		// Designers may keep tuning the BP, so preserve the reduced-size contract instead of pinning one art value.
		TestTrue(
			TEXT("Ground Hands visual Blueprint keeps a reduced positive scale"),
			VisualDefaults->GetHandVisualScale() > 0.0f && VisualDefaults->GetHandVisualScale() < 1.0f);
	}

	return true;
}

#endif
