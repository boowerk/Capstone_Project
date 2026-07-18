#if WITH_DEV_AUTOMATION_TESTS

#include "AI/Tasks/BossAttackPatternSelector.h"
#include "Actors/GP_DarkKnightChargeActor.h"
#include "Actors/GP_DarkKnightGroundCrackActor.h"
#include "Actors/GP_DarkWaveProjectile.h"
#include "Characters/GP_DarkArmorKnightBossCharacter.h"
#include "Characters/GP_DarkArmorKnightStateComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "GameplayTags/GP_Tags.h"
#include "Misc/AutomationTest.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"
#include "VFX/GP_BossTelegraphVFXComponent.h"

namespace DarkArmorKnightBossTests
{
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
	Context.DistanceToTarget = 300.0f;
	Context.BossPhase = 1;
	Context.bCanUseDarkKnightBasic = true;
	ExpectPattern(*this, Context, GPTags::Ability::Boss::DarkKnight::Basic, TEXT("Close target uses sword combo"));

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
	Context.bGuardBroken = true;
	ExpectPattern(*this, Context, GPTags::Ability::Boss::DarkKnight::Groggy, TEXT("Broken guard interrupts with groggy"));

	Context.bGuardBroken = false;
	Context.bIsGuarding = true;
	const TArray<FGPBossAttackPatternCandidate> GuardCandidates = FGPBossAttackPatternSelector::BuildCandidates(Context);
	TestTrue(TEXT("Guard stance suppresses overlapping attacks"), GuardCandidates.IsEmpty());
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

	TestNotNull(TEXT("Boss uses the supplied knightBoss skeletal mesh"), Boss->GetMesh()->GetSkeletalMeshAsset());
	AGP_DarkWaveProjectile* Wave = TestWorld->SpawnActor<AGP_DarkWaveProjectile>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
	AGP_DarkKnightGroundCrackActor* Crack = TestWorld->SpawnActor<AGP_DarkKnightGroundCrackActor>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
	TestNotNull(TEXT("Sword wave has a replaceable primitive mesh"), IsValid(Wave) ? Wave->FindComponentByClass<UStaticMeshComponent>() : nullptr);
	TestNotNull(TEXT("Ground crack has a replaceable primitive mesh"), IsValid(Crack) ? Crack->FindComponentByClass<UStaticMeshComponent>() : nullptr);

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

	TestNotNull(TEXT("Charge actor owns a Niagara telegraph component"), TelegraphVFX);
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
			TEXT("Charge uses the requested lightning telegraph asset"),
			TelegraphSystem->GetPathName(),
			FString(TEXT("/Game/Niagara/Vefects/Easy_Impact_Frames/VFX/Extras/Particles/NS_Extra_Lightning_Example_VFX.NS_Extra_Lightning_Example_VFX")));
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

#endif
