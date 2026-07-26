#if WITH_DEV_AUTOMATION_TESTS

#include "Animation/PDA_CharacterAnimationSet.h"
#include "Animation/AnimMontage.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPPlayerProductionRollMontageTest,
	"ProjectEden.Player.Roll.ProductionMontage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPPlayerProductionRollMontageTest::RunTest(const FString& Parameters)
{
	const UPDA_CharacterAnimationSet* AnimationSet =
		LoadObject<UPDA_CharacterAnimationSet>(
			nullptr,
			TEXT("/Game/Characters/MaskMan/PDA_MaskMan_AnimationSet.PDA_MaskMan_AnimationSet"));
	TestNotNull(TEXT("MaskMan production animation set loads"), AnimationSet);
	if (!AnimationSet)
	{
		return false;
	}

	const UAnimMontage* RollMontage = AnimationSet->RollMontages.Roll_RM;
	TestNotNull(TEXT("MaskMan uses a native root-motion roll montage"), RollMontage);
	if (RollMontage)
	{
		TestTrue(
			TEXT("Production roll montage contains root motion"),
			RollMontage->HasRootMotion());
		TestEqual(
			TEXT("MaskMan roll resolves to the production native montage"),
			RollMontage->GetPathName(),
			FString(
				TEXT("/Game/Characters/MaskMan/Montages/AM_MaskMan_Roll_RM.AM_MaskMan_Roll_RM")));
	}

	const UAnimMontage* SourceRollMontage =
		AnimationSet->SourceRollMontages.Roll_RM;
	TestNotNull(
		TEXT("Non-MaskMan party meshes have a source roll for retargeting"),
		SourceRollMontage);
	if (SourceRollMontage)
	{
		TestTrue(
			TEXT("Retarget source roll contains root motion"),
			SourceRollMontage->HasRootMotion());
	}

	return !HasAnyErrors();
}

#endif
