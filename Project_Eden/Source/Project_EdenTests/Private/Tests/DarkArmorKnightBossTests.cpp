#if WITH_DEV_AUTOMATION_TESTS

#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/GP_AbilitySystemComponent.h"
#include "AbilitySystem/GP_AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AI/Tasks/BossAttackPatternSelector.h"
#include "Actors/GP_DarkKnightChargeActor.h"
#include "Actors/GP_DarkKnightGroundCrackActor.h"
#include "Actors/GP_DarkWaveProjectile.h"
#include "Characters/GP_DarkArmorKnightBossCharacter.h"
#include "Characters/GP_DarkArmorKnightStateComponent.h"
#include "Components/DecalComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "GameplayTags/GP_Tags.h"
#include "Misc/AutomationTest.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"
#include "VFX/GP_BossTelegraphVFXComponent.h"

namespace DarkArmorKnightBossTests
{
	const FString ExpectedKnightMeshPath =
		TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_DarkArmorKnight/SK_KnightBoss.SK_KnightBoss");

	const FGPBossAttackPatternCandidate* Select(const FGPBossAttackPatternContext& Context, TArray<FGPBossAttackPatternCandidate>& OutCandidates)
	{
		OutCandidates = FGPBossAttackPatternSelector::BuildCandidates(Context);
		return FGPBossAttackPatternSelector::SelectBestCandidate(OutCandidates);
	}

	bool ExpectPattern(FAutomationTestBase& Test, FGPBossAttackPatternContext Context, const FGameplayTag& ExpectedTag, const TCHAR* CaseName)
	{
		TArray<FGPBossAttackPatternCandidate> Candidates;
		const FGPBossAttackPatternCandidate* Selected = Select(Context, Candidates);
		return Test.TestTrue(
			FString::Printf(TEXT("%s expected %s. Candidates=%s"), CaseName, *ExpectedTag.ToString(), *FGPBossAttackPatternSelector::DescribeCandidates(Candidates)),
			Selected && Selected->AbilityTag.MatchesTagExact(ExpectedTag));
	}

	int32 CountGrantedAbilitiesWithExactTag(
		const UAbilitySystemComponent* AbilitySystemComponent,
		const FGameplayTag& AbilityTag)
	{
		int32 MatchingAbilityCount = 0;
		if (!IsValid(AbilitySystemComponent))
		{
			return MatchingAbilityCount;
		}

		for (const FGameplayAbilitySpec& AbilitySpec : AbilitySystemComponent->GetActivatableAbilities())
		{
			const UGameplayAbility* Ability = AbilitySpec.Ability;
			if (IsValid(Ability) && Ability->GetAssetTags().HasTagExact(AbilityTag))
			{
				++MatchingAbilityCount;
			}
		}
		return MatchingAbilityCount;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDarkArmorKnightPatternSelectorTest,
	"ProjectEden.AI.Boss.PatternSelector.DarkArmorKnight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDarkArmorKnightPatternSelectorTest::RunTest(const FString& Parameters)
{
	using namespace DarkArmorKnightBossTests;
	FGPBossAttackPatternContext Context;
	Context.bIsDarkArmorKnight = true;
	Context.DistanceToTarget = Context.DarkKnightBasicAttackRange;
	Context.BossPhase = 1;
	Context.bCanUseDarkKnightBasic = true;
	ExpectPattern(*this, Context, GPTags::Ability::Boss::DarkKnight::Basic, TEXT("Basic is valid at its exact cone edge"));
	Context.DistanceToTarget += 1.0f;
	TestTrue(
		TEXT("Basic is rejected one centimeter beyond its real cone"),
		FGPBossAttackPatternSelector::BuildCandidates(Context).IsEmpty());

	Context = FGPBossAttackPatternContext();
	Context.bIsDarkArmorKnight = true;
	Context.bCanUseDarkKnightHeavy = true;
	Context.DistanceToTarget = Context.DarkKnightHeavyAttackRange;
	ExpectPattern(*this, Context, GPTags::Ability::Boss::DarkKnight::Heavy, TEXT("Heavy is valid at its exact cone edge"));
	Context.DistanceToTarget += 1.0f;
	TestTrue(
		TEXT("Heavy is rejected one centimeter beyond its real cone"),
		FGPBossAttackPatternSelector::BuildCandidates(Context).IsEmpty());

	Context = FGPBossAttackPatternContext();
	Context.bIsDarkArmorKnight = true;
	Context.bCanUseDarkWave = true;
	Context.DistanceToTarget = Context.DarkKnightDarkWaveMaxRange;
	ExpectPattern(*this, Context, GPTags::Ability::Boss::DarkKnight::DarkWave, TEXT("Dark Wave is valid at its authored slash edge"));
	Context.DistanceToTarget += 1.0f;
	TestTrue(
		TEXT("Dark Wave is rejected beyond its authored slash instead of stopping at range"),
		FGPBossAttackPatternSelector::BuildCandidates(Context).IsEmpty());

	Context = FGPBossAttackPatternContext();
	Context.bIsDarkArmorKnight = true;
	Context.DistanceToTarget = 1200.0f;
	Context.bCanUseDarkKnightBasic = true;
	Context.bCanUseDarkKnightHeavy = true;
	Context.bCanUseDarkWave = true;
	TestTrue(
		TEXT("Melee and Dark Wave readiness cannot stop chase at 1200cm"),
		FGPBossAttackPatternSelector::BuildCandidates(Context).IsEmpty());

	Context = FGPBossAttackPatternContext();
	Context.bIsDarkArmorKnight = true;
	Context.bCanUseDarkKnightBasic = false;
	Context.bCanUseDarkKnightSweep = true;
	Context.LastHitDirection = TEXT("Side");
	const TArray<FGPBossAttackPatternCandidate> RetiredSweepCandidates = FGPBossAttackPatternSelector::BuildCandidates(Context);
	// Sweep is reserved as animation wind-up and must not re-enter the damage selector.
	TestTrue(TEXT("Retired sweep tag does not create a selectable pattern"), RetiredSweepCandidates.IsEmpty());

	Context = FGPBossAttackPatternContext();
	Context.bIsDarkArmorKnight = true;
	Context.DistanceToTarget = 1200.0f;
	Context.bCanUseDarkKnightCharge = true;
	Context.bCanUseDarkWave = true;
	ExpectPattern(*this, Context, GPTags::Ability::Boss::DarkKnight::Charge, TEXT("Far target prioritizes armored charge"));

	Context = FGPBossAttackPatternContext();
	Context.bIsDarkArmorKnight = true;
	Context.DistanceToTarget = 1200.0f;
	Context.bCanUseGroundCrack = true;
	ExpectPattern(*this, Context, GPTags::Ability::Boss::DarkKnight::GroundCrack, TEXT("Ground Crack remains a ranged pattern"));

	Context = FGPBossAttackPatternContext();
	Context.bIsDarkArmorKnight = true;
	Context.bGuardBroken = true;
	ExpectPattern(*this, Context, GPTags::Ability::Boss::DarkKnight::Groggy, TEXT("Broken guard interrupts with groggy"));

	Context.bGuardBroken = false;
	Context.bIsGuarding = true;
	const TArray<FGPBossAttackPatternCandidate> GuardCandidates = FGPBossAttackPatternSelector::BuildCandidates(Context);
	TestTrue(TEXT("Guard stance suppresses overlapping attacks"), GuardCandidates.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDarkArmorKnightAbilityGrantContractTest,
	"ProjectEden.Combat.DarkArmorKnight.ProductionAbilityGrantContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDarkArmorKnightAbilityGrantContractTest::RunTest(const FString& Parameters)
{
	using namespace DarkArmorKnightBossTests;

	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Created production ability grant test world"), TestWorld))
	{
		return false;
	}

	UClass* ProductionBossClass = LoadClass<AGP_DarkArmorKnightBossCharacter>(
		nullptr,
		TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_DarkArmorKnight/BP_DarkArmorKnight.BP_DarkArmorKnight_C"));
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGP_DarkArmorKnightBossCharacter* Boss = IsValid(ProductionBossClass)
		? TestWorld->SpawnActor<AGP_DarkArmorKnightBossCharacter>(
			ProductionBossClass,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			Params)
		: nullptr;
	ATargetPoint* Target = TestWorld->SpawnActor<ATargetPoint>(
		FVector(300.0f, 0.0f, 0.0f),
		FRotator::ZeroRotator,
		Params);
	if (!TestNotNull(TEXT("Loaded production Dark Knight Blueprint"), ProductionBossClass)
		|| !TestNotNull(TEXT("Spawned production Dark Knight Blueprint"), Boss)
		|| !TestNotNull(TEXT("Spawned ability target"), Target))
	{
		TestWorld->DestroyWorld(false);
		return false;
	}

	const USkeletalMesh* ProductionKnightMesh = Boss->GetMesh()->GetSkeletalMeshAsset();
	TestNotNull(TEXT("Production Dark Knight owns its body skeletal mesh"), ProductionKnightMesh);
	if (IsValid(ProductionKnightMesh))
	{
		TestEqual(
			TEXT("Production Dark Knight uses the relocated body skeletal mesh"),
			ProductionKnightMesh->GetPathName(),
			ExpectedKnightMeshPath);
	}

	UGP_AbilitySystemComponent* ASC = Cast<UGP_AbilitySystemComponent>(Boss->GetAbilitySystemComponent());
	UGP_DarkArmorKnightStateComponent* State = Boss->GetDarkKnightStateComponent();
	if (!TestNotNull(TEXT("Production Dark Knight owns the project ASC"), ASC)
		|| !TestNotNull(TEXT("Production Dark Knight owns its state component"), State))
	{
		TestWorld->DestroyWorld(false);
		return false;
	}

	ASC->InitAbilityActorInfo(Boss, Boss);
	State->InitializeDarkKnightState(Boss);
	// Call twice to lock idempotency as well as recovery from the production Blueprint's serialized empty array.
	Boss->GrantDarkKnightAbilities();
	Boss->GrantDarkKnightAbilities();

	const FGameplayTag RequiredAbilityTags[] =
	{
		GPTags::Ability::Boss::DarkKnight::Basic,
		GPTags::Ability::Boss::DarkKnight::Heavy,
		GPTags::Ability::Boss::DarkKnight::Charge,
		GPTags::Ability::Boss::DarkKnight::DarkWave,
		GPTags::Ability::Boss::DarkKnight::GroundCrack,
		GPTags::Ability::Boss::DarkKnight::Groggy,
	};
	for (const FGameplayTag& RequiredAbilityTag : RequiredAbilityTags)
	{
		TestEqual(
			FString::Printf(TEXT("%s has exactly one granted spec"), *RequiredAbilityTag.ToString()),
			CountGrantedAbilitiesWithExactTag(ASC, RequiredAbilityTag),
			1);
	}

	// Keep the routing assertion independent of animation evaluation; this test proves GAS reaches the authored hit timer.
	Boss->PreAttackMontage = nullptr;
	Boss->TelegraphVFXPatterns.FindOrAdd(GPTags::Ability::Boss::DarkKnight::Basic) = false;
	Boss->BeginBehaviorAttackCommit(Target, 8.0f);
	// A shorter designer-tuned slash is valid, but the obsolete 2200cm range must never return.
	TestTrue(
		TEXT("Production Blueprint cannot retain the obsolete 2200cm Dark Wave range"),
		Boss->GetDarkWaveMaxRange() <= FGPBossAttackPatternRanges::DarkKnightDarkWaveMaxRange);
	Target->SetActorLocation(FVector(Boss->GetBasicAttackRange() + 1.0f, 0.0f, 0.0f));
	TestFalse(
		TEXT("Basic tag activation is rejected outside its real damage range"),
		ASC->TryActivateAbilityByTag(GPTags::Ability::Boss::DarkKnight::Basic));
	TestTrue(TEXT("Rejected Basic does not consume shared cadence"), Boss->CanStartDarkKnightPattern());
	Target->SetActorLocation(FVector(Boss->GetBasicAttackRange(), 0.0f, 0.0f));
	const bool bBasicActivated = ASC->TryActivateAbilityByTag(GPTags::Ability::Boss::DarkKnight::Basic);
	TestTrue(TEXT("Production Dark Knight activates Basic by exact tag"), bBasicActivated);
	TestTrue(
		TEXT("Basic tag activation reaches the attack implementation"),
		Boss->LastPatternUseTimes.Contains(GPTags::Ability::Boss::DarkKnight::Basic));
	TestTrue(TEXT("Basic activation schedules its authored impact"), !Boss->PatternTimerHandles.IsEmpty());

	Boss->FinishBehaviorAttackCommit(0.0f);
	Boss->ClearPatternTimers();
	TestWorld->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDarkArmorKnightGuardLifecycleTest,
	"ProjectEden.Combat.DarkArmorKnight.GuardLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDarkArmorKnightGuardLifecycleTest::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Created Dark Knight test world"), TestWorld))
	{
		return false;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGP_DarkArmorKnightBossCharacter* Boss = TestWorld->SpawnActor<AGP_DarkArmorKnightBossCharacter>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
	// TargetPoint owns a scene root, so directional hit tests can move the attacker reliably in a transient world.
	ATargetPoint* Attacker = TestWorld->SpawnActor<ATargetPoint>(FVector(300.0f, 0.0f, 0.0f), FRotator::ZeroRotator, Params);
	if (!TestNotNull(TEXT("Spawned Dark Knight boss"), Boss) || !TestNotNull(TEXT("Spawned test attacker"), Attacker))
	{
		TestWorld->DestroyWorld(false);
		return false;
	}

	UGP_DarkArmorKnightStateComponent* State = Boss->GetDarkKnightStateComponent();
	TestNotNull(TEXT("Boss owns guard state component"), State);
	State->InitializeDarkKnightState(Boss);
	TestTrue(TEXT("Guard stance starts"), State->StartGuardStance(3.0f));
	TestEqual(TEXT("Front guard reduces damage to 25 percent"), State->ResolveIncomingDamageMultiplier(Attacker, false), 0.25f);
	TestEqual(TEXT("Front hit removes eight guard points"), State->GetGuardGauge(), 92.0f);

	Attacker->SetActorLocation(FVector(0.0f, 300.0f, 0.0f));
	TestEqual(TEXT("Side guard uses 65 percent multiplier"), State->ResolveIncomingDamageMultiplier(Attacker, false), 0.65f);
	TestEqual(TEXT("Side hit removes fourteen guard points"), State->GetGuardGauge(), 78.0f);

	Attacker->SetActorLocation(FVector(-300.0f, 0.0f, 0.0f));
	TestEqual(TEXT("Back guard uses 90 percent multiplier"), State->ResolveIncomingDamageMultiplier(Attacker, false), 0.9f);
	TestEqual(TEXT("Back hit removes twenty-two guard points"), State->GetGuardGauge(), 56.0f);

	Attacker->SetActorLocation(FVector(300.0f, 0.0f, 0.0f));
	TestTrue(TEXT("Parry window opens only during guard"), State->StartParryWindow(0.45f));
	TestEqual(TEXT("Front parry cancels all incoming damage"), State->ResolveIncomingDamageMultiplier(Attacker, false), 0.0f);
	TestFalse(TEXT("Successful parry consumes the window"), State->IsParryWindowOpen());

	State->ResolveIncomingDamageMultiplier(Attacker, true);
	State->ResolveIncomingDamageMultiplier(Attacker, true);
	TestTrue(TEXT("Heavy hits break the remaining guard and enter groggy"), State->IsGroggy());
	TestEqual(TEXT("Groggy increases received damage"), State->ResolveIncomingDamageMultiplier(Attacker, false), 1.2f);
	State->RecoverFromGroggy();
	TestFalse(TEXT("Recovery clears groggy"), State->IsGroggy());
	TestEqual(TEXT("Recovery restores guard gauge"), State->GetGuardGauge(), State->GetMaxGuardGauge());

	const USkeletalMesh* NativeKnightMesh = Boss->GetMesh()->GetSkeletalMeshAsset();
	TestNotNull(TEXT("Boss uses the supplied knightBoss skeletal mesh"), NativeKnightMesh);
	if (IsValid(NativeKnightMesh))
	{
		TestEqual(
			TEXT("Native Dark Knight uses the current knightBoss asset path"),
			NativeKnightMesh->GetPathName(),
			DarkArmorKnightBossTests::ExpectedKnightMeshPath);
	}
	AGP_DarkWaveProjectile* Wave = TestWorld->SpawnActor<AGP_DarkWaveProjectile>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
	AGP_DarkKnightGroundCrackActor* Crack = TestWorld->SpawnActor<AGP_DarkKnightGroundCrackActor>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
	const UNiagaraComponent* WaveVFX = IsValid(Wave)
		? Wave->FindComponentByClass<UNiagaraComponent>()
		: nullptr;
	const UDecalComponent* CrackDecal = IsValid(Crack)
		? Crack->FindComponentByClass<UDecalComponent>()
		: nullptr;
	TestNull(
		TEXT("Sword wave no longer exposes an engine primitive mesh"),
		IsValid(Wave) ? Wave->FindComponentByClass<UStaticMeshComponent>() : nullptr);
	TestNotNull(TEXT("Sword wave owns a Niagara presentation"), WaveVFX);
	if (IsValid(WaveVFX) && IsValid(WaveVFX->GetAsset()))
	{
		TestEqual(
			TEXT("Sword wave uses the sprite-only dark projectile system"),
			WaveVFX->GetAsset()->GetPathName(),
			FString(TEXT("/Game/Mixed_Magic_VFX_Pack/VFX/NS_Dark_Solo_Projectile.NS_Dark_Solo_Projectile")));
	}
	TestNull(
		TEXT("Ground crack no longer exposes an engine primitive mesh"),
		IsValid(Crack) ? Crack->FindComponentByClass<UStaticMeshComponent>() : nullptr);
	TestNotNull(TEXT("Ground crack owns a terrain-conforming warning decal"), CrackDecal);
	if (IsValid(CrackDecal) && IsValid(CrackDecal->GetDecalMaterial()))
	{
		TestEqual(
			TEXT("Ground crack uses the emissive combat telegraph material"),
			CrackDecal->GetDecalMaterial()->GetPathName(),
			FString(TEXT("/Game/Effects/M_EmissiveCircleTelegraph_Decal.M_EmissiveCircleTelegraph_Decal")));
	}

	TestWorld->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDarkArmorKnightCommittedTargetResolutionTest,
	"ProjectEden.Combat.DarkArmorKnight.CommittedTargetResolution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDarkArmorKnightCommittedTargetResolutionTest::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Created committed-target test world"), TestWorld))
	{
		return false;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGP_DarkArmorKnightBossCharacter* Boss = TestWorld->SpawnActor<AGP_DarkArmorKnightBossCharacter>(
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		Params);
	ATargetPoint* CommittedTarget = TestWorld->SpawnActor<ATargetPoint>(
		FVector(0.0f, 500.0f, 0.0f),
		FRotator::ZeroRotator,
		Params);
	ATargetPoint* ExplicitTarget = TestWorld->SpawnActor<ATargetPoint>(
		FVector(0.0f, -500.0f, 0.0f),
		FRotator::ZeroRotator,
		Params);
	if (!TestNotNull(TEXT("Spawned Dark Knight"), Boss)
		|| !TestNotNull(TEXT("Spawned committed target"), CommittedTarget)
		|| !TestNotNull(TEXT("Spawned explicit target"), ExplicitTarget))
	{
		TestWorld->DestroyWorld(false);
		return false;
	}

	// Ability activation does not pass event target data, so the actor must
	// recover the exact target that the latent BT task committed.
	Boss->BeginBehaviorAttackCommit(CommittedTarget, 8.0f);
	TestTrue(TEXT("Basic attack resolves a missing explicit target"), Boss->ExecuteBasicAttack(nullptr));
	TestTrue(
		TEXT("Missing explicit target keeps the committed player direction"),
		FMath::IsNearlyEqual(Boss->GetActorRotation().Yaw, 90.0f, 1.0f));

	Boss->SetActorRotation(FRotator::ZeroRotator);
	TestTrue(TEXT("Explicit counter-style target still takes priority"), Boss->ExecuteBasicAttack(ExplicitTarget));
	TestTrue(
		TEXT("Explicit target overrides the committed fallback"),
		FMath::IsNearlyEqual(Boss->GetActorRotation().Yaw, -90.0f, 1.0f));

	Boss->FinishBehaviorAttackCommit(0.0f);
	TestWorld->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDarkArmorKnightChargeTelegraphVFXTest,
	"ProjectEden.Combat.DarkArmorKnight.ChargeTelegraphVFX",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDarkArmorKnightChargeTelegraphVFXTest::RunTest(const FString& Parameters)
{
	// Keep the lightning warning on the replicated charge coordinator instead of server-only spawned presentation.
	const AGP_DarkKnightChargeActor* ChargeDefaults = GetDefault<AGP_DarkKnightChargeActor>();
	const UGP_BossTelegraphVFXComponent* TelegraphVFX = IsValid(ChargeDefaults)
		? ChargeDefaults->FindComponentByClass<UGP_BossTelegraphVFXComponent>()
		: nullptr;
	const UNiagaraSystem* TelegraphSystem = IsValid(TelegraphVFX)
		? TelegraphVFX->GetDefaultTelegraphSystem()
		: nullptr;
	const UDecalComponent* TelegraphDecal = IsValid(ChargeDefaults)
		? ChargeDefaults->FindComponentByClass<UDecalComponent>()
		: nullptr;

	TestNotNull(TEXT("Charge actor owns a Niagara telegraph component"), TelegraphVFX);
	TestNull(
		TEXT("Charge telegraph no longer exposes an engine primitive mesh"),
		IsValid(ChargeDefaults) ? ChargeDefaults->FindComponentByClass<UStaticMeshComponent>() : nullptr);
	TestNotNull(TEXT("Charge keeps its authored range as a ground decal"), TelegraphDecal);
	if (IsValid(TelegraphDecal) && IsValid(TelegraphDecal->GetDecalMaterial()))
	{
		TestEqual(
			TEXT("Charge lane uses the emissive combat telegraph material"),
			TelegraphDecal->GetDecalMaterial()->GetPathName(),
			FString(TEXT("/Game/Effects/M_EmissiveCircleTelegraph_Decal.M_EmissiveCircleTelegraph_Decal")));
	}
	TestNotNull(TEXT("Charge telegraph component loads the lightning system"), TelegraphSystem);
	if (IsValid(TelegraphVFX))
	{
		TestTrue(TEXT("Legacy charge telegraph explicitly opts into automatic playback"), TelegraphVFX->IsTelegraphVFXEnabled());
		TestTrue(TEXT("Charge lightning auto-activates when the coordinator spawns"), TelegraphVFX->bAutoActivate);
		TestEqual(TEXT("Charge lightning uses the enlarged reusable scale"), TelegraphVFX->GetUniformVisualScale(), 1.35f);
		TestEqual(TEXT("Charge waits for the component telegraph duration"), TelegraphVFX->GetTelegraphDuration(), 0.9f);
	}
	if (IsValid(TelegraphSystem))
	{
		TestEqual(
			TEXT("Charge uses the sprite-only lightning telegraph asset"),
			TelegraphSystem->GetPathName(),
			FString(TEXT("/Game/Mixed_Magic_VFX_Pack/VFX/NS_Lightning_Owner_Cast.NS_Lightning_Owner_Cast")));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDarkArmorKnightChargeMovementFallbackTest,
	"ProjectEden.Combat.DarkArmorKnight.ChargeMovementFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDarkArmorKnightChargeMovementFallbackTest::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Created charge-fallback test world"), TestWorld))
	{
		return false;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGP_DarkArmorKnightBossCharacter* Boss = TestWorld->SpawnActor<AGP_DarkArmorKnightBossCharacter>(
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		Params);
	ATargetPoint* Target = TestWorld->SpawnActor<ATargetPoint>(
		FVector(3000.0f, 0.0f, 0.0f),
		FRotator::ZeroRotator,
		Params);
	AGP_DarkKnightChargeActor* Charge = TestWorld->SpawnActor<AGP_DarkKnightChargeActor>(
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		Params);
	if (!TestNotNull(TEXT("Spawned native Dark Knight"), Boss)
		|| !TestNotNull(TEXT("Spawned distant charge target"), Target)
		|| !TestNotNull(TEXT("Spawned charge coordinator"), Charge))
	{
		TestWorld->DestroyWorld(false);
		return false;
	}

	// The native test actor deliberately has no AnimInstance/montage playback,
	// reproducing the product failure mode without editing production assets.
	TestFalse(
		TEXT("Native test boss cannot start the charge montage"),
		Boss->PlayPatternMontage(GPTags::Ability::Boss::DarkKnight::Charge));
	const FVector StartLocation = Boss->GetActorLocation();
	Charge->InitializeCharge(Boss, Target);
	// Transient automation worlds do not advance gameplay timers like PIE.
	TestWorld->GetTimerManager().ClearTimer(Charge->TelegraphTimerHandle);
	Charge->StartCharge();
	Charge->Tick(0.1f);

	const float TravelDistance = FVector::Dist2D(StartLocation, Boss->GetActorLocation());
	TestTrue(
		TEXT("Failed charge montage falls back to visible swept travel"),
		TravelDistance >= 200.0f);
	TestTrue(
		TEXT("Fallback charge remains active before reaching its target or cap"),
		IsValid(Charge) && !Charge->IsActorBeingDestroyed());

	TestWorld->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDarkArmorKnightGroggyInterruptTest,
	"ProjectEden.Combat.DarkArmorKnight.GroggyInterruptsPatterns",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDarkArmorKnightGroggyInterruptTest::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Created groggy-interrupt test world"), TestWorld))
	{
		return false;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGP_DarkArmorKnightBossCharacter* Boss = TestWorld->SpawnActor<AGP_DarkArmorKnightBossCharacter>(
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		Params);
	ATargetPoint* Target = TestWorld->SpawnActor<ATargetPoint>(
		FVector(1400.0f, 0.0f, 0.0f),
		FRotator::ZeroRotator,
		Params);
	if (!TestNotNull(TEXT("Spawned Dark Knight for groggy interrupt"), Boss)
		|| !TestNotNull(TEXT("Spawned groggy interrupt target"), Target))
	{
		TestWorld->DestroyWorld(false);
		return false;
	}

	UGP_DarkArmorKnightStateComponent* State = Boss->GetDarkKnightStateComponent();
	UAbilitySystemComponent* ASC = Boss->GetAbilitySystemComponent();
	UGP_AttributeSet* Attributes =
		const_cast<UGP_AttributeSet*>(Cast<UGP_AttributeSet>(Boss->GetAttributeSet()));
	if (!TestNotNull(TEXT("Dark Knight owns its state component"), State)
		|| !TestNotNull(TEXT("Dark Knight owns its ability system"), ASC)
		|| !TestNotNull(TEXT("Dark Knight owns its attribute set"), Attributes))
	{
		TestWorld->DestroyWorld(false);
		return false;
	}
	// A transient world skips the normal component initialization pass, so
	// register the existing subobject explicitly before setting phase health.
	if (!ASC->GetSpawnedAttributes().Contains(Attributes))
	{
		ASC->AddAttributeSetSubobject(Attributes);
	}
	ASC->InitAbilityActorInfo(Boss, Boss);
	ASC->SetNumericAttributeBase(UGP_AttributeSet::GetMaxHealthAttribute(), 100.0f);
	ASC->SetNumericAttributeBase(UGP_AttributeSet::GetHealthAttribute(), 20.0f);
	State->InitializeDarkKnightState(Boss);
	TestEqual(TEXT("Test boss enters phase three"), Boss->GetDarkKnightPhase(), 3);

	// Prove the phase-three follow-up can be scheduled before testing its interrupt gate.
	Boss->HandleChargeFinished(false, Target);
	TestEqual(TEXT("Healthy phase-three charge schedules one follow-up"), Boss->PatternTimerHandles.Num(), 1);
	Boss->ClearPatternTimers();

	TestTrue(TEXT("Heavy attack schedules a delayed impact"), Boss->ExecuteHeavyAttack(Target));
	const FTimerHandle PendingImpactHandle = Boss->PatternTimerHandles.Last();
	TestTrue(
		TEXT("Heavy impact timer exists before groggy"),
		TestWorld->GetTimerManager().TimerExists(PendingImpactHandle));
	TestTrue(TEXT("Charge starts before guard break"), Boss->ExecuteChargeAttack(Target));
	AGP_DarkKnightChargeActor* SpawnedCharge = Boss->ActiveChargeActor.Get();
	TestNotNull(TEXT("Boss tracks the active charge coordinator"), SpawnedCharge);

	State->EnterGroggy(8.0f);
	// BeginPlay delegate binding is intentionally skipped in this narrow
	// lifecycle fixture; invoke the same authoritative callback directly.
	Boss->HandleGroggyChanged(true);

	TestTrue(TEXT("Groggy clears all boss-owned pattern timers"), Boss->PatternTimerHandles.IsEmpty());
	TestFalse(
		TEXT("Groggy removes the pending heavy impact timer"),
		TestWorld->GetTimerManager().TimerExists(PendingImpactHandle));
	TestTrue(
		TEXT("Groggy destroys the active charge coordinator"),
		!IsValid(SpawnedCharge) || SpawnedCharge->IsActorBeingDestroyed());
	TestFalse(TEXT("Boss releases its active charge reference"), Boss->ActiveChargeActor.IsValid());

	Boss->HandleChargeFinished(false, Target);
	TestTrue(
		TEXT("Groggy phase three cannot schedule a charge follow-up"),
		Boss->PatternTimerHandles.IsEmpty());

	TestWorld->DestroyWorld(false);
	return true;
}

#endif
