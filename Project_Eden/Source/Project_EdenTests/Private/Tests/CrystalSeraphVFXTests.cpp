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

	const FLinearColor ExpectedCrystalTint(89.f / 255.f, 173.f / 255.f, 1.f, 1.f);
	const auto HasCrystalSeraphTint = [ExpectedCrystalTint](const UGP_VisualCueComponent* VisualCueComponent) -> bool
	{
		return VisualCueComponent
			&& VisualCueComponent->IsNiagaraTintOverrideEnabled()
			&& VisualCueComponent->GetNiagaraTintOverrideColor().Equals(ExpectedCrystalTint, 0.001f);
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

	TestNull(TEXT("Native prism leaves aura VFX for its Blueprint child"), PrismAura);
	TestNull(TEXT("Native prism leaves reflection VFX for its Blueprint child"), PrismReflect);
	TestNull(TEXT("Native shard leaves active VFX for its Blueprint child"), ShardActive);
	TestNull(TEXT("Native shard leaves impact VFX for its Blueprint child"), ShardImpact);
	TestNull(TEXT("Native laser leaves telegraph VFX for its Blueprint child"), LaserTelegraph);
	TestNull(TEXT("Native laser leaves active VFX for its Blueprint child"), LaserActive);
	TestNull(TEXT("Native laser leaves reflection VFX for its Blueprint child"), LaserReflect);
	TestNull(TEXT("Native sanctuary leaves telegraph VFX for its Blueprint child"), SanctuaryTelegraph);
	TestNull(TEXT("Native sanctuary leaves explosion VFX for its Blueprint child"), SanctuaryImpact);
	// Crystal Seraph actors tint their copied Niagara components at spawn time so the original Free_Magic assets remain untouched.
	TestTrue(TEXT("Prism applies Crystal Seraph tint"), HasCrystalSeraphTint(PrismCues));
	TestTrue(TEXT("Shard applies Crystal Seraph tint"), HasCrystalSeraphTint(ShardCues));
	TestTrue(TEXT("Laser applies Crystal Seraph tint"), HasCrystalSeraphTint(LaserCues));
	TestTrue(TEXT("Sanctuary applies Crystal Seraph tint"), HasCrystalSeraphTint(SanctuaryCues));

	TInlineComponentArray<UGP_VisualCueComponent*> LaserCueComponents;
	PatternActors[2]->GetComponents(LaserCueComponents);
	TestEqual(TEXT("Laser owns primary and reflection-beam VFX cue components"), LaserCueComponents.Num(), 2);
	for (const UGP_VisualCueComponent* LaserCueComponent : LaserCueComponents)
	{
		TestTrue(TEXT("Every laser VFX cue applies Crystal Seraph tint"), HasCrystalSeraphTint(LaserCueComponent));
	}

	TestWorld->DestroyWorld(false);
	return true;
}

#endif
