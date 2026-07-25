#if WITH_DEV_AUTOMATION_TESTS

#include "Actors/GP_BossSweepTelegraphActor.h"

#include "Components/DecalComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBossSweepUsesFloorDecalTest,
	"ProjectEden.AI.Boss.Sweep.UsesFloorDecal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBossSweepUsesFloorDecalTest::RunTest(const FString& Parameters)
{
	// Prevent the production warning from regressing to editor/debug drawing primitives.
	const AGP_BossSweepTelegraphActor* TelegraphActor = GetDefault<AGP_BossSweepTelegraphActor>();
	const UDecalComponent* WarningDecal = IsValid(TelegraphActor)
		? TelegraphActor->FindComponentByClass<UDecalComponent>()
		: nullptr;
	const UMaterialInterface* DecalMaterial = IsValid(WarningDecal)
		? WarningDecal->GetDecalMaterial()
		: nullptr;
	const UMaterial* BaseMaterial = IsValid(DecalMaterial) ? DecalMaterial->GetMaterial() : nullptr;

	TestNotNull(TEXT("Sweep telegraph owns a floor decal component"), WarningDecal);
	TestNotNull(TEXT("Sweep telegraph decal has a material"), DecalMaterial);
	if (IsValid(WarningDecal))
	{
		TestEqual(
			TEXT("Sweep telegraph decal projects downward"),
			WarningDecal->GetRelativeRotation(),
			FRotator(-90.0f, -90.0f, 0.0f));
	}
	if (IsValid(DecalMaterial))
	{
		TestEqual(
			TEXT("Sweep telegraph uses the dedicated fan decal material"),
			DecalMaterial->GetPathName(),
			FString(TEXT("/Game/Effects/M_BossSweepTelegraph_Decal.M_BossSweepTelegraph_Decal")));
	}
	TestNotNull(TEXT("Sweep telegraph resolves a base material"), BaseMaterial);
	if (IsValid(BaseMaterial))
	{
		TestEqual(TEXT("Sweep telegraph material is a deferred decal"), BaseMaterial->MaterialDomain, MD_DeferredDecal);
	}

	return true;
}

#endif
