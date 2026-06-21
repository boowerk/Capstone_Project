#if WITH_DEV_AUTOMATION_TESTS

#include "Actors/GP_CrystalPrismActor.h"
#include "Actors/GP_CrystalSanctuaryMarkerActor.h"
#include "Actors/GP_CrystalShardProjectile.h"
#include "Actors/GP_SeraphLaserActor.h"
#include "Engine/World.h"
#include "GameplayTags/GP_Tags.h"
#include "Misc/AutomationTest.h"
#include "NiagaraSystem.h"
#include "VFX/GP_VisualCueComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCrystalSeraphVisualCueTest,
	"ProjectEden.Combat.CrystalSeraph.VisualCues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCrystalSeraphVisualCueTest::RunTest(const FString& Parameters)
{
	UNiagaraSystem* GenericSystem = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Imported_VFX/Free_Magic/VFX_Niagara/NS_Free_Magic_Aura.NS_Free_Magic_Aura"));
	UNiagaraSystem* ExactSystem = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Imported_VFX/Free_Magic/VFX_Niagara/NS_Free_Magic_Hit1.NS_Free_Magic_Hit1"));
	if (!TestNotNull(TEXT("Loaded generic Niagara fixture"), GenericSystem) || !TestNotNull(TEXT("Loaded exact Niagara fixture"), ExactSystem))
	{
		return false;
	}

	UGP_VisualCueComponent* ResolverFixture = NewObject<UGP_VisualCueComponent>();
	ResolverFixture->AddNiagaraCue(GPTags::GameplayCue::Ability::Active_Magic, GenericSystem);
	ResolverFixture->AddNiagaraCue(GPTags::GameplayCue::Ability::Impact_Magic, ExactSystem);
	// Boss actors must resolve cue-specific entries with the same exact-match rule as player SkillData.
	TestEqual(TEXT("Exact impact cue wins over other cue entries"), ResolverFixture->ResolveNiagara(GPTags::GameplayCue::Ability::Impact_Magic), ExactSystem);

	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Created Crystal Seraph VFX test world"), TestWorld))
	{
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const TArray<AActor*> PatternActors =
	{
		TestWorld->SpawnActor<AGP_CrystalPrismActor>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters),
		TestWorld->SpawnActor<AGP_CrystalShardProjectile>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters),
		TestWorld->SpawnActor<AGP_SeraphLaserActor>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters),
		TestWorld->SpawnActor<AGP_CrystalSanctuaryMarkerActor>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters),
	};

	for (AActor* PatternActor : PatternActors)
	{
		TestNotNull(TEXT("Spawned Crystal Seraph pattern actor"), PatternActor);
		TestNotNull(TEXT("Pattern actor owns reusable visual cue component"), IsValid(PatternActor) ? PatternActor->FindComponentByClass<UGP_VisualCueComponent>() : nullptr);
	}

	const UGP_VisualCueComponent* PrismCues = PatternActors[0]->FindComponentByClass<UGP_VisualCueComponent>();
	const UGP_VisualCueComponent* ShardCues = PatternActors[1]->FindComponentByClass<UGP_VisualCueComponent>();
	const UGP_VisualCueComponent* LaserCues = PatternActors[2]->FindComponentByClass<UGP_VisualCueComponent>();
	const UGP_VisualCueComponent* SanctuaryCues = PatternActors[3]->FindComponentByClass<UGP_VisualCueComponent>();
	TestNotNull(TEXT("Prism has persistent aura VFX"), PrismCues->ResolveNiagara(GPTags::GameplayCue::Ability::Active_Magic));
	TestNotNull(TEXT("Prism has reflection burst VFX"), PrismCues->ResolveNiagara(GPTags::GameplayCue::Ability::Reflect_Magic));
	TestNotNull(TEXT("Shard has active projectile VFX"), ShardCues->ResolveNiagara(GPTags::GameplayCue::Ability::Active_Magic));
	TestNotNull(TEXT("Shard has impact VFX"), ShardCues->ResolveNiagara(GPTags::GameplayCue::Ability::Impact_Magic));
	TestNotNull(TEXT("Laser has telegraph VFX"), LaserCues->ResolveNiagara(GPTags::GameplayCue::Ability::Telegraph_Magic));
	TestNotNull(TEXT("Laser has active VFX"), LaserCues->ResolveNiagara(GPTags::GameplayCue::Ability::Active_Magic));
	TestNotNull(TEXT("Laser has reflection VFX"), LaserCues->ResolveNiagara(GPTags::GameplayCue::Ability::Reflect_Magic));
	TestNotNull(TEXT("Sanctuary has telegraph VFX"), SanctuaryCues->ResolveNiagara(GPTags::GameplayCue::Ability::Telegraph_Magic));
	TestNotNull(TEXT("Sanctuary has explosion VFX"), SanctuaryCues->ResolveNiagara(GPTags::GameplayCue::Ability::Impact_Magic));

	TestWorld->DestroyWorld(false);
	return true;
}

#endif
