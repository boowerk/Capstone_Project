#if WITH_DEV_AUTOMATION_TESTS

#include "Characters/GP_PlayerCharacter.h"
#include "Misc/AutomationTest.h"
#include "NiagaraSystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPPlayerLifeStateProductionContractTest,
	"ProjectEden.Player.LifeState.ProductionContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPPlayerLifeStateProductionContractTest::RunTest(const FString& Parameters)
{
	const UClass* PlayerClass = LoadClass<AGP_PlayerCharacter>(
		nullptr,
		TEXT(
			"/Game/Characters/PlayerCharacter/"
			"BP_GP_PlayerCharacter.BP_GP_PlayerCharacter_C"));
	const AGP_PlayerCharacter* PlayerDefaults = PlayerClass
		? Cast<AGP_PlayerCharacter>(PlayerClass->GetDefaultObject())
		: nullptr;
	if (!TestNotNull(TEXT("Production player Blueprint loads"), PlayerDefaults))
	{
		return false;
	}

	TestTrue(
		TEXT("Player pawns stay relevant across distant party villages for spectating"),
		PlayerDefaults->bAlwaysRelevant);
	TestNotNull(
		TEXT("Production player has an elimination scatter system"),
		PlayerDefaults->EliminationVFX.Get());
	if (PlayerDefaults->EliminationVFX)
	{
		TestEqual(
			TEXT("Player elimination uses the runtime-bindable skeletal scatter system"),
			PlayerDefaults->EliminationVFX->GetPathName(),
			FString(
				TEXT(
					"/Game/Niagara/Dissolve_SK/"
					"NS_EnemyDeath_Absorb.NS_EnemyDeath_Absorb")));
	}
	TestTrue(
		TEXT("Niagara samples the player before the source mesh is hidden"),
		PlayerDefaults->EliminationMeshHideDelay
			> PlayerDefaults->EliminationVFXEmissionTime);

	return !HasAnyErrors();
}

#endif
