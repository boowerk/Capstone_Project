#if WITH_DEV_AUTOMATION_TESTS

#include "Actors/GP_CrystalPrismActor.h"
#include "Characters/GP_CrystalSeraphBossCharacter.h"
#include "Characters/GP_CrystalSeraphStateComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "Engine/TargetPoint.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCrystalSeraphGroggyLifecycleTest,
	"ProjectEden.Combat.CrystalSeraph.GroggyLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCrystalSeraphGroggyLifecycleTest::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Created transient Crystal Seraph test world"), TestWorld))
	{
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGP_CrystalSeraphBossCharacter* Boss = TestWorld->SpawnActor<AGP_CrystalSeraphBossCharacter>(
		FVector(0.0f, 0.0f, 1000.0f),
		FRotator::ZeroRotator,
		SpawnParameters);
	AGP_CrystalPrismActor* Prism = TestWorld->SpawnActor<AGP_CrystalPrismActor>(
		FVector(300.0f, 0.0f, 100.0f),
		FRotator::ZeroRotator,
		SpawnParameters);
	if (!TestNotNull(TEXT("Spawned Crystal Seraph"), Boss) || !TestNotNull(TEXT("Spawned reflection prism"), Prism))
	{
		TestWorld->DestroyWorld(false);
		return false;
	}

	UGP_CrystalSeraphStateComponent* StateComponent = Boss->GetCrystalSeraphStateComponent();
	if (!TestNotNull(TEXT("Crystal Seraph owns its state component"), StateComponent))
	{
		TestWorld->DestroyWorld(false);
		return false;
	}

	StateComponent->InitializeCrystalSeraphState(Boss);
	Prism->InitializePrism(Boss);
	// Reflection success is counted once per laser pattern and reaches groggy exactly on the third hit.
	TestTrue(TEXT("First reflected laser is accepted"), Prism->NotifyLaserHit(nullptr, FVector::ForwardVector));
	TestEqual(TEXT("First laser breaks one stage"), StateComponent->GetWingCoreBreakCount(), 1);
	TestTrue(TEXT("Second reflected laser is accepted"), Prism->NotifyLaserHit(nullptr, FVector::ForwardVector));
	TestEqual(TEXT("Second laser breaks two stages"), StateComponent->GetWingCoreBreakCount(), 2);
	TestFalse(TEXT("Boss remains airborne before the third hit"), StateComponent->IsGroggy());
	TestTrue(TEXT("Third reflected laser is accepted"), Prism->NotifyLaserHit(nullptr, FVector::ForwardVector));
	TestTrue(TEXT("Third laser enters groggy"), StateComponent->IsGroggy());

	// Exercise the boss-owned presentation and recovery gate without requiring BeginPlay in the transient world.
	Boss->HandleCrystalSeraphGroggyChanged(true);
	TestEqual(TEXT("Groggy starts as a physical fall"), Boss->GetCharacterMovement()->MovementMode, MOVE_Falling);
	TestFalse(TEXT("Recovery is not scheduled before player damage"), Boss->IsGroggyRecoveryScheduled());

	APlayerState* PlayerDamageInstigator = TestWorld->SpawnActor<APlayerState>(
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParameters);
	if (TestNotNull(TEXT("Spawned player damage instigator"), PlayerDamageInstigator))
	{
		Boss->HandlePostDamageTaken(PlayerDamageInstigator, 10.0f, FGameplayTag());
		TestFalse(TEXT("A player hit during the fall does not schedule recovery"), Boss->IsGroggyRecoveryScheduled());

		// Landing changes the temporary fall mode to walking before the required vulnerability hit can count.
		Boss->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		Boss->HandlePostDamageTaken(PlayerDamageInstigator, 10.0f, FGameplayTag());
		TestTrue(TEXT("First post-landing player hit schedules recovery"), Boss->IsGroggyRecoveryScheduled());
		TestTrue(TEXT("Recovery delay owns an active timer"), TestWorld->GetTimerManager().IsTimerActive(Boss->GroggyRecoveryTimerHandle));

		// The timer callback uses the same recovery entry point that restores guarded airborne combat.
		Boss->RequestRecoverFromGroggy();
		TestFalse(TEXT("Recovery clears groggy"), StateComponent->IsGroggy());
		TestFalse(TEXT("Recovery consumes the scheduled gate"), Boss->IsGroggyRecoveryScheduled());
		TestEqual(TEXT("Recovered boss returns to flying movement"), Boss->GetCharacterMovement()->MovementMode, MOVE_Flying);
	}

	TestWorld->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCrystalSeraphPrismClusterTest,
	"ProjectEden.Combat.CrystalSeraph.PrismCluster",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCrystalSeraphPrismClusterTest::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Created prism cluster test world"), TestWorld))
	{
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGP_CrystalSeraphBossCharacter* Boss = TestWorld->SpawnActor<AGP_CrystalSeraphBossCharacter>(
		FVector(0.0f, 0.0f, 1000.0f),
		FRotator::ZeroRotator,
		SpawnParameters);
	ATargetPoint* Target = TestWorld->SpawnActor<ATargetPoint>(
		FVector(1000.0f, 0.0f, 0.0f),
		FRotator::ZeroRotator,
		SpawnParameters);
	if (!TestNotNull(TEXT("Spawned cluster boss"), Boss) || !TestNotNull(TEXT("Spawned cluster target"), Target))
	{
		TestWorld->DestroyWorld(false);
		return false;
	}

	UGP_CrystalSeraphStateComponent* StateComponent = Boss->GetCrystalSeraphStateComponent();
	StateComponent->InitializeCrystalSeraphState(Boss);
	AActor* PrimaryPrism = Boss->RequestSpawnCrystalPrism(Target);
	TestNotNull(TEXT("Prism pattern returns a primary representative"), PrimaryPrism);
	TestEqual(TEXT("State tracks the primary prism"), StateComponent->GetCrystalPrismActor(), PrimaryPrism);

	// Native actors cover the cluster mechanics below. Production visuals live in the Blueprint child,
	// so verify that the production boss selects that child and that its authored Sculpture scale remains large.
	UClass* ProductionBossClass = LoadClass<AGP_CrystalSeraphBossCharacter>(nullptr,
		TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_CrystalSeraph/BP_Crystal_Seraph.BP_Crystal_Seraph_C"));
	UClass* ProductionPrismClass = LoadClass<AGP_CrystalPrismActor>(nullptr,
		TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_CrystalSeraph/BP_CrystalPrism.BP_CrystalPrism_C"));
	const AGP_CrystalSeraphBossCharacter* ProductionBossDefaults = IsValid(ProductionBossClass)
		? Cast<AGP_CrystalSeraphBossCharacter>(ProductionBossClass->GetDefaultObject())
		: nullptr;
	const FClassProperty* PrismClassProperty = FindFProperty<FClassProperty>(
		AGP_CrystalSeraphBossCharacter::StaticClass(),
		TEXT("CrystalPrismActorClass"));
	const UClass* ConfiguredPrismClass = IsValid(ProductionBossDefaults) && PrismClassProperty
		? Cast<UClass>(PrismClassProperty->GetObjectPropertyValue_InContainer(ProductionBossDefaults))
		: nullptr;
	TestNotNull(TEXT("Loaded production Crystal Seraph Blueprint"), ProductionBossClass);
	TestNotNull(TEXT("Loaded production Crystal Prism Blueprint"), ProductionPrismClass);
	TestTrue(TEXT("Production boss selects the production prism Blueprint"),
		ConfiguredPrismClass == ProductionPrismClass);

	const AGP_CrystalPrismActor* ProductionPrismDefaults = IsValid(ProductionPrismClass)
		? Cast<AGP_CrystalPrismActor>(ProductionPrismClass->GetDefaultObject())
		: nullptr;
	const FStructProperty* PrismScaleProperty = FindFProperty<FStructProperty>(
		AGP_CrystalPrismActor::StaticClass(),
		TEXT("PrismVisualScale"));
	const FVector* ProductionPrismScale = IsValid(ProductionPrismDefaults) && PrismScaleProperty
		? PrismScaleProperty->ContainerPtrToValuePtr<FVector>(ProductionPrismDefaults)
		: nullptr;
	TestTrue(TEXT("Production prism preserves its authored large visual scale"),
		ProductionPrismScale && ProductionPrismScale->GetMin() >= 2.0f);

	const UStaticMeshComponent* ProductionPrismMesh = nullptr;
	if (IsValid(ProductionPrismDefaults))
	{
		TInlineComponentArray<UStaticMeshComponent*> ProductionMeshComponents;
		ProductionPrismDefaults->GetComponents(ProductionMeshComponents);
		for (const UStaticMeshComponent* MeshComponent : ProductionMeshComponents)
		{
			if (IsValid(MeshComponent) && MeshComponent->GetFName() == TEXT("PrismMesh"))
			{
				ProductionPrismMesh = MeshComponent;
				break;
			}
		}
	}
	TestTrue(TEXT("Production prism owns its authored static mesh"),
		IsValid(ProductionPrismMesh) && IsValid(ProductionPrismMesh->GetStaticMesh()));

	TArray<AActor*> PrismActors;
	UGameplayStatics::GetAllActorsOfClass(TestWorld, AGP_CrystalPrismActor::StaticClass(), PrismActors);
	TestEqual(TEXT("Prism pattern lays three crystals"), PrismActors.Num(), 3);
	for (int32 LeftIndex = 0; LeftIndex < PrismActors.Num(); ++LeftIndex)
	{
		const AGP_CrystalPrismActor* PrismActor = Cast<AGP_CrystalPrismActor>(PrismActors[LeftIndex]);
		const USphereComponent* PrismCollision = PrismActors[LeftIndex]->FindComponentByClass<USphereComponent>();
		TestTrue(TEXT("Crystal retains its enlarged reflection radius"), IsValid(PrismCollision) && PrismCollision->GetUnscaledSphereRadius() >= 150.0f);
		TestTrue(TEXT("Crystal remains upright after ring placement"), IsValid(PrismActor) && FMath::IsNearlyZero(PrismActor->GetActorRotation().Pitch) && FMath::IsNearlyZero(PrismActor->GetActorRotation().Roll));
		TestTrue(TEXT("Persistent aura stays smaller than the crystal prototype"), IsValid(PrismActor) && PrismActor->GetPrismAuraScale().GetMax() < 1.0f);

		for (int32 RightIndex = LeftIndex + 1; RightIndex < PrismActors.Num(); ++RightIndex)
		{
			// Ring placement must keep the enlarged crystals visibly separate.
			TestTrue(TEXT("Prism cluster points do not overlap"), FVector::Dist2D(PrismActors[LeftIndex]->GetActorLocation(), PrismActors[RightIndex]->GetActorLocation()) > 500.0f);
		}
	}

	TestWorld->DestroyWorld(false);
	return true;
}

#endif
