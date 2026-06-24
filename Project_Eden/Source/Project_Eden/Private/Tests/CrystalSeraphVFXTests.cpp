#if WITH_DEV_AUTOMATION_TESTS

#include "Actors/GP_CrystalPrismActor.h"
#include "Actors/GP_CrystalSanctuaryMarkerActor.h"
#include "Actors/GP_CrystalShardProjectile.h"
#include "Actors/GP_CrystalSeraphVFXDefaults.h"
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

	const auto IsCrystalSeraphCopy = [](const UNiagaraSystem* NiagaraSystem) -> bool
	{
		return NiagaraSystem
			&& NiagaraSystem->GetPathName().Contains(TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_CrystalSeraph/VFX/NS_CrystalSeraph_"));
	};
	const auto HasCrystalSeraphTint = [](const UGP_VisualCueComponent* VisualCueComponent) -> bool
	{
		return VisualCueComponent
			&& VisualCueComponent->IsNiagaraTintOverrideEnabled()
			&& VisualCueComponent->GetNiagaraTintOverrideColor().Equals(GPCrystalSeraphVFXDefaults::GetCrystalTintColor(), 0.001f);
	};

	UNiagaraSystem* PrismAura = PrismCues->ResolveNiagara(GPTags::GameplayCue::Ability::Active_Magic);
	UNiagaraSystem* PrismReflect = PrismCues->ResolveNiagara(GPTags::GameplayCue::Ability::Reflect_Magic);
	UNiagaraSystem* ShardActive = ShardCues->ResolveNiagara(GPTags::GameplayCue::Ability::Active_Magic);
	UNiagaraSystem* ShardImpact = ShardCues->ResolveNiagara(GPTags::GameplayCue::Ability::Impact_Magic);
	UNiagaraSystem* LaserTelegraph = LaserCues->ResolveNiagara(GPTags::GameplayCue::Ability::Telegraph_Magic);
	UNiagaraSystem* LaserActive = LaserCues->ResolveNiagara(GPTags::GameplayCue::Ability::Active_Magic);
	UNiagaraSystem* LaserReflect = LaserCues->ResolveNiagara(GPTags::GameplayCue::Ability::Reflect_Magic);
	UNiagaraSystem* SanctuaryTelegraph = SanctuaryCues->ResolveNiagara(GPTags::GameplayCue::Ability::Telegraph_Magic);
	UNiagaraSystem* SanctuaryImpact = SanctuaryCues->ResolveNiagara(GPTags::GameplayCue::Ability::Impact_Magic);

	TestTrue(TEXT("Prism uses Crystal Seraph aura VFX copy"), IsCrystalSeraphCopy(PrismAura));
	TestTrue(TEXT("Prism uses Crystal Seraph reflection VFX copy"), IsCrystalSeraphCopy(PrismReflect));
	TestTrue(TEXT("Shard uses Crystal Seraph projectile VFX copy"), IsCrystalSeraphCopy(ShardActive));
	TestTrue(TEXT("Shard uses Crystal Seraph impact VFX copy"), IsCrystalSeraphCopy(ShardImpact));
	TestTrue(TEXT("Laser uses Crystal Seraph telegraph VFX copy"), IsCrystalSeraphCopy(LaserTelegraph));
	TestTrue(TEXT("Laser uses Crystal Seraph active VFX copy"), IsCrystalSeraphCopy(LaserActive));
	TestTrue(TEXT("Laser uses Crystal Seraph reflection VFX copy"), IsCrystalSeraphCopy(LaserReflect));
	TestTrue(TEXT("Sanctuary uses Crystal Seraph telegraph VFX copy"), IsCrystalSeraphCopy(SanctuaryTelegraph));
	TestTrue(TEXT("Sanctuary uses Crystal Seraph explosion VFX copy"), IsCrystalSeraphCopy(SanctuaryImpact));
	// Crystal Seraph actors tint their copied Niagara components at spawn time so the original Free_Magic assets remain untouched.
	TestTrue(TEXT("Prism applies Crystal Seraph tint"), HasCrystalSeraphTint(PrismCues));
	TestTrue(TEXT("Shard applies Crystal Seraph tint"), HasCrystalSeraphTint(ShardCues));
	TestTrue(TEXT("Laser applies Crystal Seraph tint"), HasCrystalSeraphTint(LaserCues));
	TestTrue(TEXT("Sanctuary applies Crystal Seraph tint"), HasCrystalSeraphTint(SanctuaryCues));

	TestWorld->DestroyWorld(false);
	return true;
}

#endif
