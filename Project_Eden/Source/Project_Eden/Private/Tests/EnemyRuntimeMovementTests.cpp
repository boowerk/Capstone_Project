#if WITH_DEV_AUTOMATION_TESTS

#include "Characters/GP_DarkArmorKnightBossCharacter.h"
#include "Characters/GP_EnemyCharacter.h"
#include "Characters/GP_MatadorMageBossCharacter.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEnemyRuntimeMovementPolicyTest,
	"ProjectEden.AI.Enemy.RuntimeMovementPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyRuntimeMovementPolicyTest::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Created enemy movement policy test world"), TestWorld))
	{
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	UClass* FurnaceClass = LoadClass<AGP_EnemyCharacter>(
		nullptr,
		TEXT("/Game/Characters/EnemyCharacter/Monsters/FurnaceWalker/BP_FurnaceWalker.BP_FurnaceWalker_C"));
	AGP_EnemyCharacter* Furnace = IsValid(FurnaceClass)
		? TestWorld->SpawnActor<AGP_EnemyCharacter>(FurnaceClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters)
		: nullptr;
	if (TestNotNull(TEXT("Loaded and spawned production FurnaceWalker"), Furnace))
	{
		Furnace->ApplyRuntimeMovementPolicy();
		TestTrue(TEXT("Turn-enabled FurnaceWalker keeps its runtime Tick"), Furnace->IsActorTickEnabled());
		const UCharacterMovementComponent* FurnaceMovement = Furnace->GetCharacterMovement();
		TestTrue(
			TEXT("Regular FurnaceWalker keeps path-facing chase rotation"),
			IsValid(FurnaceMovement) && FurnaceMovement->bOrientRotationToMovement);
	}

	AGP_DarkArmorKnightBossCharacter* DarkKnight = TestWorld->SpawnActor<AGP_DarkArmorKnightBossCharacter>(
		FVector(500.0f, 0.0f, 0.0f),
		FRotator::ZeroRotator,
		SpawnParameters);
	if (TestNotNull(TEXT("Spawned native Dark Armor Knight"), DarkKnight))
	{
		DarkKnight->ApplyRuntimeMovementPolicy();
		TestFalse(TEXT("Dark Armor Knight does not enable an unused actor Tick"), DarkKnight->IsActorTickEnabled());
		const UCharacterMovementComponent* BossMovement = DarkKnight->GetCharacterMovement();
		TestTrue(
			TEXT("Dark Armor Knight retains boss-owned facing"),
			IsValid(BossMovement) && !BossMovement->bOrientRotationToMovement);
	}

	const AGP_MatadorMageBossCharacter* MatadorDefaults = GetDefault<AGP_MatadorMageBossCharacter>();
	TestTrue(
		TEXT("Matador explicitly requests its persistent main-body Tick"),
		IsValid(MatadorDefaults) && MatadorDefaults->PrimaryActorTick.bStartWithTickEnabled);

	AGP_MatadorMageBossCharacter* Matador = TestWorld->SpawnActor<AGP_MatadorMageBossCharacter>(
		FVector(1000.0f, 0.0f, 0.0f),
		FRotator::ZeroRotator,
		SpawnParameters);
	if (TestNotNull(TEXT("Spawned native Matador"), Matador))
	{
		// Exercise only the merge policy; full BeginPlay would spawn decoys and grant unrelated abilities.
		Matador->ApplyRuntimeMovementPolicy();
		TestTrue(TEXT("Matador preserves its main-body and decoy-follow Tick"), Matador->IsActorTickEnabled());
		const UCharacterMovementComponent* MatadorMovement = Matador->GetCharacterMovement();
		TestTrue(
			TEXT("Matador retains boss-owned facing"),
			IsValid(MatadorMovement) && !MatadorMovement->bOrientRotationToMovement);
	}

	TestWorld->DestroyWorld(false);
	return !HasAnyErrors();
}

#endif
