#if WITH_DEV_AUTOMATION_TESTS

#include "Actors/GP_BossDeathPresentationActor.h"
#include "Characters/GP_CrystalSeraphBossCharacter.h"
#include "Characters/GP_DarkArmorKnightBossCharacter.h"
#include "Characters/GP_EnemyCharacter.h"
#include "Characters/GP_MatadorMageBossCharacter.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "VFX/GP_BossDeathPresentationComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBossDeathPresentationConfigurationTest,
	"ProjectEden.Combat.Boss.DeathPresentation.Configuration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBossDeathPresentationConfigurationTest::RunTest(const FString& Parameters)
{
	const AGP_EnemyCharacter* EnemyDefaults = GetDefault<AGP_EnemyCharacter>();
	const AGP_CrystalSeraphBossCharacter* CrystalDefaults = GetDefault<AGP_CrystalSeraphBossCharacter>();
	const AGP_DarkArmorKnightBossCharacter* DarkKnightDefaults = GetDefault<AGP_DarkArmorKnightBossCharacter>();
	const AGP_MatadorMageBossCharacter* MatadorDefaults = GetDefault<AGP_MatadorMageBossCharacter>();

	const UGP_BossDeathPresentationComponent* EnemyPresentation = EnemyDefaults->GetBossDeathPresentationComponent();
	const UGP_BossDeathPresentationComponent* CrystalPresentation = CrystalDefaults->GetBossDeathPresentationComponent();
	const UGP_BossDeathPresentationComponent* DarkKnightPresentation = DarkKnightDefaults->GetBossDeathPresentationComponent();
	const UGP_BossDeathPresentationComponent* MatadorPresentation = MatadorDefaults->GetBossDeathPresentationComponent();

	TestNotNull(TEXT("Enemy base owns the dormant boss death presentation component"), EnemyPresentation);
	TestNotNull(TEXT("Crystal Seraph owns the boss death presentation component"), CrystalPresentation);
	TestNotNull(TEXT("Dark Armor Knight owns the boss death presentation component"), DarkKnightPresentation);
	TestNotNull(TEXT("Matador owns the boss death presentation component"), MatadorPresentation);

	TestEqual(TEXT("Generic enemy auto style resolves to no boss presentation"),
		IsValid(EnemyPresentation) ? EnemyPresentation->ResolvePresentationStyle() : EGPBossDeathPresentationStyle::Auto,
		EGPBossDeathPresentationStyle::None);
	TestEqual(TEXT("Crystal Seraph default selects shatter presentation"),
		IsValid(CrystalPresentation) ? CrystalPresentation->GetConfiguredPresentationStyle() : EGPBossDeathPresentationStyle::None,
		EGPBossDeathPresentationStyle::CrystalSeraph);
	TestEqual(TEXT("Dark Armor Knight default selects armor lightning presentation"),
		IsValid(DarkKnightPresentation) ? DarkKnightPresentation->GetConfiguredPresentationStyle() : EGPBossDeathPresentationStyle::None,
		EGPBossDeathPresentationStyle::DarkArmorKnight);
	TestEqual(TEXT("Matador default selects bull arena presentation"),
		IsValid(MatadorPresentation) ? MatadorPresentation->GetConfiguredPresentationStyle() : EGPBossDeathPresentationStyle::None,
		EGPBossDeathPresentationStyle::Matador);
	TestFalse(
		TEXT("Boss presentation leaves source-mesh dissolve and hide to the shared absorption component"),
		IsValid(CrystalPresentation) && CrystalPresentation->DoesPresentationHideSourceMesh());

	TestEqual(TEXT("Sans Blueprint names auto-map to the hand-and-crack presentation"),
		UGP_BossDeathPresentationComponent::ResolveAutoPresentationStyleFromName(
			TEXT("BP_Boss_Sans_C_0"),
			FText::FromString(TEXT("Sans"))),
		EGPBossDeathPresentationStyle::Sans);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBossDeathPresentationActorPiecesTest,
	"ProjectEden.Combat.Boss.DeathPresentation.Pieces",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBossDeathPresentationActorPiecesTest::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Created boss death presentation test world"), TestWorld))
	{
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const EGPBossDeathPresentationStyle StylesToVerify[] =
	{
		EGPBossDeathPresentationStyle::CrystalSeraph,
		EGPBossDeathPresentationStyle::Sans,
		EGPBossDeathPresentationStyle::DarkArmorKnight,
		EGPBossDeathPresentationStyle::Matador,
	};

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(StylesToVerify); ++Index)
	{
		AGP_BossDeathPresentationActor* PresentationActor = TestWorld->SpawnActor<AGP_BossDeathPresentationActor>(
			FVector(Index * 500.0f, 0.0f, 100.0f),
			FRotator::ZeroRotator,
			SpawnParameters);
		if (!TestNotNull(FString::Printf(TEXT("Spawned presentation actor %d"), Index), PresentationActor))
		{
			TestWorld->DestroyWorld(false);
			return false;
		}

		FGPBossDeathPresentationSpawnSettings Settings;
		Settings.bHideSourceMesh = false;
		// Keep tests independent from any optional Niagara asset; piece generation is the required native fallback.
		Settings.OverrideBurstNiagara = nullptr;
		PresentationActor->InitializePresentation(StylesToVerify[Index], nullptr, nullptr, Settings);

		TestEqual(TEXT("Presentation actor records its initialized style"), PresentationActor->GetPresentationStyle(), StylesToVerify[Index]);
		TestTrue(TEXT("Each boss presentation creates visible fallback pieces"), PresentationActor->GetSpawnedPieceCount() > 0);
		TestTrue(TEXT("Presentation actor remains alive long enough to outlast boss despawn"), PresentationActor->GetLifeSpan() > 0.0f);
	}

	TestWorld->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBossDeathPresentationComponentPlaybackTest,
	"ProjectEden.Combat.Boss.DeathPresentation.ComponentPlayback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBossDeathPresentationComponentPlaybackTest::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Created boss death presentation component test world"), TestWorld))
	{
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGP_EnemyCharacter* GenericEnemy = TestWorld->SpawnActor<AGP_EnemyCharacter>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
	AGP_CrystalSeraphBossCharacter* CrystalBoss = TestWorld->SpawnActor<AGP_CrystalSeraphBossCharacter>(
		FVector(600.0f, 0.0f, 120.0f),
		FRotator::ZeroRotator,
		SpawnParameters);

	if (!TestNotNull(TEXT("Spawned generic enemy"), GenericEnemy) || !TestNotNull(TEXT("Spawned Crystal Seraph boss"), CrystalBoss))
	{
		TestWorld->DestroyWorld(false);
		return false;
	}

	UGP_BossDeathPresentationComponent* GenericPresentation = GenericEnemy->GetBossDeathPresentationComponent();
	UGP_BossDeathPresentationComponent* CrystalPresentation = CrystalBoss->GetBossDeathPresentationComponent();
	TestFalse(TEXT("Generic enemy component refuses to play boss clear presentation"),
		IsValid(GenericPresentation) && GenericPresentation->PlayDeathPresentation(nullptr));
	TestTrue(TEXT("Boss component plays its local clear presentation"),
		IsValid(CrystalPresentation) && CrystalPresentation->PlayDeathPresentation(nullptr));
	TestFalse(TEXT("Boss presentation is idempotent after the first play"),
		IsValid(CrystalPresentation) && CrystalPresentation->PlayDeathPresentation(nullptr));

	TArray<AActor*> SpawnedPresentations;
	UGameplayStatics::GetAllActorsOfClass(TestWorld, AGP_BossDeathPresentationActor::StaticClass(), SpawnedPresentations);
	TestEqual(TEXT("Exactly one boss death presentation actor was spawned"), SpawnedPresentations.Num(), 1);

	TestWorld->DestroyWorld(false);
	return true;
}

#endif
