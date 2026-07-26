#if WITH_DEV_AUTOMATION_TESTS

#include "Actors/GP_SkillTargetPreviewActor.h"

#include "Components/DecalComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialInterface.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSkillTargetPreviewUsesEmissiveDecalTest,
	"ProjectEden.Combat.SkillTargetPreview.UsesEmissiveDecal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSkillTargetPreviewUsesEmissiveDecalTest::RunTest(const FString& Parameters)
{
	const AGP_SkillTargetPreviewActor* PreviewDefaults = GetDefault<AGP_SkillTargetPreviewActor>();
	const UDecalComponent* AreaDecal = IsValid(PreviewDefaults)
		? PreviewDefaults->FindComponentByClass<UDecalComponent>()
		: nullptr;
	const UMaterialInterface* DecalMaterial = IsValid(AreaDecal)
		? AreaDecal->GetDecalMaterial()
		: nullptr;
	const UMaterial* BaseMaterial = IsValid(DecalMaterial)
		? DecalMaterial->GetMaterial()
		: nullptr;

	TestNotNull(TEXT("Skill target preview owns a range decal"), AreaDecal);
	TestNotNull(TEXT("Skill target preview range decal has a material"), DecalMaterial);
	if (IsValid(DecalMaterial))
	{
		TestEqual(
			TEXT("Skill target preview uses the dedicated emissive material"),
			DecalMaterial->GetPathName(),
			FString(TEXT("/Game/Effects/M_EmissiveCircleTelegraph_Decal.M_EmissiveCircleTelegraph_Decal")));
	}

	TestNotNull(TEXT("Skill target preview resolves the emissive base material"), BaseMaterial);
	if (IsValid(BaseMaterial))
	{
		TestEqual(
			TEXT("Skill target preview material projects as a deferred decal"),
			BaseMaterial->MaterialDomain,
			MD_DeferredDecal);

		float EmissiveStrength = 0.0f;
		const bool bHasEmissiveStrength = BaseMaterial->GetScalarParameterDefaultValue(
			FHashedMaterialParameterInfo(TEXT("TelegraphEmissiveStrength")),
			EmissiveStrength);
		TestTrue(TEXT("Range decal exposes an emissive strength parameter"), bHasEmissiveStrength);
		TestTrue(TEXT("Range decal remains visible without scene lighting"), EmissiveStrength > 0.0f);

		float TelegraphOpacity = 0.0f;
		const bool bHasOpacity = BaseMaterial->GetScalarParameterDefaultValue(
			FHashedMaterialParameterInfo(TEXT("TelegraphOpacity")),
			TelegraphOpacity);
		TestTrue(TEXT("Range decal exposes its opacity"), bHasOpacity);
		TestTrue(TEXT("Range decal has a visible default opacity"), TelegraphOpacity >= 0.35f);

#if WITH_EDITORONLY_DATA
		const FExpressionInput* EmissiveInput =
			const_cast<UMaterial*>(BaseMaterial)->GetExpressionInputForProperty(MP_EmissiveColor);
		TestTrue(
			TEXT("Range decal graph connects a source to Emissive Color"),
			EmissiveInput != nullptr && IsValid(EmissiveInput->Expression));

		bool bHasLightingIndependentMarker = false;
		for (const TObjectPtr<UMaterialExpression>& Expression : BaseMaterial->GetExpressions())
		{
			if (IsValid(Expression.Get())
				&& Expression->Desc == TEXT("LIGHTING_INDEPENDENT_TELEGRAPH_EMISSIVE"))
			{
				bHasLightingIndependentMarker = true;
				break;
			}
		}
		TestTrue(
			TEXT("Range decal keeps the lighting-independent emissive graph"),
			bHasLightingIndependentMarker);
#endif
	}

	return true;
}

#endif
