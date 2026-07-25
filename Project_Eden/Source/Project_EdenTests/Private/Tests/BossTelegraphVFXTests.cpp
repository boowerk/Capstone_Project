#if WITH_DEV_AUTOMATION_TESTS

#include "Characters/GP_CrystalSeraphBossCharacter.h"
#include "Characters/GP_DarkArmorKnightBossCharacter.h"
#include "Characters/GP_EnemyCharacter.h"
#include "Characters/GP_MatadorMageBossCharacter.h"
#include "Engine/World.h"
#include "GameplayTags/GP_Tags.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"
#include "VFX/GP_BossTelegraphVFXComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBossTelegraphVFXConfigurationTest,
	"ProjectEden.Combat.Boss.TelegraphVFXConfiguration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBossTelegraphVFXConfigurationTest::RunTest(const FString& Parameters)
{
	// The opt-in switch must remain a designer-editable bool and contribute no delay while disabled.
	const FBoolProperty* ToggleProperty = FindFProperty<FBoolProperty>(
		UGP_BossTelegraphVFXComponent::StaticClass(),
		TEXT("bTelegraphVFXEnabled"));
	TestNotNull(TEXT("Telegraph VFX toggle is a bool property"), ToggleProperty);
	TestTrue(TEXT("Telegraph VFX toggle is editable in Blueprint Details"),
		ToggleProperty && ToggleProperty->HasAnyPropertyFlags(CPF_Edit));

	UGP_BossTelegraphVFXComponent* ComponentFixture = NewObject<UGP_BossTelegraphVFXComponent>();
	TestFalse(TEXT("Boss telegraph is opt-in by default"), ComponentFixture->IsTelegraphVFXEnabled());
	TestEqual(TEXT("Disabled telegraph adds no pattern delay"), ComponentFixture->GetEnabledTelegraphDuration(), 0.0f);
	ComponentFixture->SetTelegraphVFXEnabled(true);
	TestEqual(TEXT("Enabled telegraph exposes its configured lead time"),
		ComponentFixture->GetEnabledTelegraphDuration(), ComponentFixture->GetTelegraphDuration());
	TMap<FGameplayTag, bool> PatternSelectionFixture;
	PatternSelectionFixture.Add(GPTags::Ability::Boss::DarkKnight::Basic, false);
	PatternSelectionFixture.Add(GPTags::Ability::Boss::DarkKnight::Charge, true);
	TestFalse(TEXT("Master on does not enable an unchecked pattern"),
		ComponentFixture->IsPatternTelegraphEnabled(GPTags::Ability::Boss::DarkKnight::Basic, PatternSelectionFixture));
	TestTrue(TEXT("Master on enables only the checked pattern"),
		ComponentFixture->IsPatternTelegraphEnabled(GPTags::Ability::Boss::DarkKnight::Charge, PatternSelectionFixture));
	TestFalse(TEXT("Unlisted patterns remain disabled"),
		ComponentFixture->IsPatternTelegraphEnabled(GPTags::Ability::Boss::DarkKnight::Heavy, PatternSelectionFixture));

	const AGP_CrystalSeraphBossCharacter* CrystalDefaults = GetDefault<AGP_CrystalSeraphBossCharacter>();
	const AGP_MatadorMageBossCharacter* MatadorDefaults = GetDefault<AGP_MatadorMageBossCharacter>();
	const AGP_DarkArmorKnightBossCharacter* NativeDarkKnightDefaults = GetDefault<AGP_DarkArmorKnightBossCharacter>();
	const AGP_EnemyCharacter* GenericEnemyDefaults = GetDefault<AGP_EnemyCharacter>();
	const UGP_BossTelegraphVFXComponent* CrystalTelegraph = CrystalDefaults->GetBossTelegraphVFXComponent();
	const UGP_BossTelegraphVFXComponent* MatadorTelegraph = MatadorDefaults->GetBossTelegraphVFXComponent();
	TestNotNull(TEXT("Crystal Seraph owns an inherited Boss Telegraph VFX component"), CrystalTelegraph);
	TestNotNull(TEXT("Matador owns an inherited Boss Telegraph VFX component"), MatadorTelegraph);
	TestFalse(TEXT("Crystal Seraph telegraph does not auto-play on spawn"), IsValid(CrystalTelegraph) && CrystalTelegraph->bAutoActivate);
	TestFalse(TEXT("Matador telegraph does not auto-play on spawn"), IsValid(MatadorTelegraph) && MatadorTelegraph->bAutoActivate);
	TestEqual(TEXT("Crystal Seraph exposes all four damaging patterns"), CrystalDefaults->GetTelegraphVFXPatterns().Num(), 4);
	TestEqual(TEXT("Matador exposes Bull, Rapier, and Cape patterns"), MatadorDefaults->GetTelegraphVFXPatterns().Num(), 3);
	TestTrue(TEXT("Crystal Laser is present and unchecked by default"),
		CrystalDefaults->GetTelegraphVFXPatterns().Contains(GPTags::Ability::Boss::CrystalSeraph::Laser)
		&& !CrystalDefaults->GetTelegraphVFXPatterns().FindRef(GPTags::Ability::Boss::CrystalSeraph::Laser));
	TestTrue(TEXT("Matador Bull is present and unchecked by default"),
		MatadorDefaults->GetTelegraphVFXPatterns().Contains(GPTags::Ability::Enemy::Utility_MatadorBullPattern)
		&& !MatadorDefaults->GetTelegraphVFXPatterns().FindRef(GPTags::Ability::Enemy::Utility_MatadorBullPattern));
	TestEqual(TEXT("Native Dark Knight exposes all eight attack patterns"),
		NativeDarkKnightDefaults->GetTelegraphVFXPatterns().Num(),
		8);
	TestTrue(TEXT("Native Dark Knight Charge is listed and disabled by default"),
		NativeDarkKnightDefaults->GetTelegraphVFXPatterns().Contains(GPTags::Ability::Boss::DarkKnight::Charge)
		&& !NativeDarkKnightDefaults->GetTelegraphVFXPatterns().FindRef(GPTags::Ability::Boss::DarkKnight::Charge));
	TestNull(TEXT("Generic enemy base does not add the component to Sans"),
		GenericEnemyDefaults->FindComponentByClass<UGP_BossTelegraphVFXComponent>());

	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Created boss telegraph test world"), TestWorld))
	{
		return false;
	}

	// Dark Knight already has a designer-added component in its Blueprint; the native integration must reuse exactly that one.
	UClass* DarkKnightClass = LoadClass<AActor>(nullptr,
		TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_DarkArmorKnight/BP_DarkArmorKnight.BP_DarkArmorKnight_C"));
	UClass* SansClass = LoadClass<AActor>(nullptr,
		TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_Sans/BP_Boss_Sans.BP_Boss_Sans_C"));
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* DarkKnight = IsValid(DarkKnightClass)
		? TestWorld->SpawnActor<AActor>(DarkKnightClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters)
		: nullptr;
	AActor* Sans = IsValid(SansClass)
		? TestWorld->SpawnActor<AActor>(SansClass, FVector(500.0f, 0.0f, 0.0f), FRotator::ZeroRotator, SpawnParameters)
		: nullptr;

	TestNotNull(TEXT("Loaded Dark Knight Blueprint"), DarkKnightClass);
	TestNotNull(TEXT("Spawned Dark Knight Blueprint"), DarkKnight);
	TestNotNull(TEXT("Loaded Sans Blueprint"), SansClass);
	TestNotNull(TEXT("Spawned Sans Blueprint"), Sans);
	TArray<UGP_BossTelegraphVFXComponent*> DarkKnightTelegraphs;
	if (IsValid(DarkKnight))
	{
		DarkKnight->GetComponents(DarkKnightTelegraphs);
	}
	TestEqual(TEXT("Dark Knight reuses one Blueprint telegraph component"), DarkKnightTelegraphs.Num(), 1);
	const AGP_DarkArmorKnightBossCharacter* DarkKnightBoss = Cast<AGP_DarkArmorKnightBossCharacter>(DarkKnight);
	TestNotNull(TEXT("Dark Knight Blueprint keeps its native boss parent"), DarkKnightBoss);
	TestEqual(TEXT("Dark Knight exposes all eight authored attack patterns"),
		IsValid(DarkKnightBoss) ? DarkKnightBoss->GetTelegraphVFXPatterns().Num() : 0,
		8);
	TestTrue(TEXT("Production Dark Knight enables the authored Charge telegraph"),
		IsValid(DarkKnightBoss)
		&& DarkKnightBoss->GetTelegraphVFXPatterns().Contains(GPTags::Ability::Boss::DarkKnight::Charge)
		&& DarkKnightBoss->GetTelegraphVFXPatterns().FindRef(GPTags::Ability::Boss::DarkKnight::Charge));
	TestNull(TEXT("Sans remains excluded from Boss Telegraph VFX"),
		IsValid(Sans) ? Sans->FindComponentByClass<UGP_BossTelegraphVFXComponent>() : nullptr);

	TestWorld->DestroyWorld(false);
	return true;
}

#endif
