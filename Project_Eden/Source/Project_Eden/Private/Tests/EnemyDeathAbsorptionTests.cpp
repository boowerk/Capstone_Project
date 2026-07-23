#if WITH_DEV_AUTOMATION_TESTS

#include "Characters/GP_EnemyCharacter.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "NiagaraDataInterfaceSkeletalMesh.h"
#include "NiagaraSystem.h"
#include "VFX/GP_EnemyDeathAbsorptionComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEnemyDeathAbsorptionPolicyTest,
	"ProjectEden.VFX.EnemyDeathAbsorption.Policy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyDeathAbsorptionPolicyTest::RunTest(const FString& Parameters)
{
	const AGP_EnemyCharacter* EnemyDefaults = GetDefault<AGP_EnemyCharacter>();
	const UGP_EnemyDeathAbsorptionComponent* ComponentDefaults =
		EnemyDefaults ? EnemyDefaults->GetEnemyDeathAbsorptionComponent() : nullptr;

	TestNotNull(TEXT("Every enemy owns the death absorption component"), ComponentDefaults);
	if (!IsValid(ComponentDefaults))
	{
		return false;
	}

	TestTrue(TEXT("The cosmetic multicast component is replicated"), ComponentDefaults->GetIsReplicated());
	TestEqual(TEXT("Source mesh parameter contract"), ComponentDefaults->SourceMeshParameterName, FName(TEXT("User.SourceMesh")));
	TestEqual(
		TEXT("Target position parameter contract"),
		ComponentDefaults->AbsorbTargetPositionParameterName,
		FName(TEXT("User.AbsorbTargetPosition")));
	TestEqual(
		TEXT("Strength parameter contract"),
		ComponentDefaults->AbsorbStrengthParameterName,
		FName(TEXT("User.AbsorbStrength")));
	TestTrue(
		TEXT("Five-second source curves are compressed inside the two-second corpse window"),
		ComponentDefaults->NiagaraPlaybackRate >= 2.5f
			&& ComponentDefaults->EffectDeactivateTimeSeconds <= 1.8f);
	TestTrue(
		TEXT("High-rate emission is stopped after a short authored burst"),
		ComponentDefaults->EmissionStopTimeSeconds * ComponentDefaults->NiagaraPlaybackRate <= 0.25f);
	TestTrue(
		TEXT("A first-frame hitch cannot stop emission before Niagara produces a visible sample"),
		ComponentDefaults->MinimumEmissionFrames >= 3);

	const float MaximumStrength = 4000.0f;
	TestEqual(
		TEXT("Scatter phase has no attraction"),
		UGP_EnemyDeathAbsorptionComponent::CalculateAbsorbStrength(0.20f, 0.20f, 0.50f, MaximumStrength),
		0.0f);
	TestTrue(
		TEXT("Attraction ramps after scatter"),
		FMath::IsNearlyEqual(
			UGP_EnemyDeathAbsorptionComponent::CalculateAbsorbStrength(0.45f, 0.20f, 0.50f, MaximumStrength),
			MaximumStrength * 0.5f,
			0.1f));
	TestEqual(
		TEXT("Attraction reaches its configured maximum"),
		UGP_EnemyDeathAbsorptionComponent::CalculateAbsorbStrength(0.90f, 0.20f, 0.50f, MaximumStrength),
		MaximumStrength);

	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Created transient selection policy world"), TestWorld))
	{
		return false;
	}

	FActorSpawnParameters NearSpawnParameters;
	NearSpawnParameters.Name = TEXT("AbsorbTarget_Near");
	NearSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* NearTarget = TestWorld->SpawnActor<AActor>(
		AActor::StaticClass(),
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		NearSpawnParameters);
	USceneComponent* NearRoot = NewObject<USceneComponent>(NearTarget, TEXT("Root"));
	NearTarget->SetRootComponent(NearRoot);
	NearRoot->RegisterComponent();
	NearTarget->SetActorLocation(FVector(200.0f, 0.0f, 0.0f));

	FActorSpawnParameters FarSpawnParameters;
	FarSpawnParameters.Name = TEXT("AbsorbTarget_Far");
	FarSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* FarTarget = TestWorld->SpawnActor<AActor>(
		AActor::StaticClass(),
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		FarSpawnParameters);
	USceneComponent* FarRoot = NewObject<USceneComponent>(FarTarget, TEXT("Root"));
	FarTarget->SetRootComponent(FarRoot);
	FarRoot->RegisterComponent();
	FarTarget->SetActorLocation(FVector(800.0f, 0.0f, 0.0f));

	TArray<AActor*> FallbackTargets = {FarTarget, NearTarget};
	TestEqual(
		TEXT("A valid killer remains preferred even when another player is nearer"),
		UGP_EnemyDeathAbsorptionComponent::SelectPreferredOrNearestTarget(
			FarTarget,
			FVector::ZeroVector,
			FallbackTargets),
		FarTarget);
	TestEqual(
		TEXT("Missing killer falls back to the nearest valid player"),
		UGP_EnemyDeathAbsorptionComponent::SelectPreferredOrNearestTarget(
			nullptr,
			FVector::ZeroVector,
			FallbackTargets),
		NearTarget);

	TestWorld->DestroyWorld(false);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEnemyDeathAbsorptionAssetContractTest,
	"ProjectEden.VFX.EnemyDeathAbsorption.ProductionAssetContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyDeathAbsorptionAssetContractTest::RunTest(const FString& Parameters)
{
	UNiagaraSystem* AbsorptionSystem = LoadObject<UNiagaraSystem>(
		nullptr,
		TEXT("/Game/Niagara/Dissolve_SK/NS_EnemyDeath_Absorb.NS_EnemyDeath_Absorb"));
	TestNotNull(TEXT("Enemy death absorption Niagara system loads"), AbsorptionSystem);
	if (!IsValid(AbsorptionSystem))
	{
		return false;
	}

	const FNiagaraUserRedirectionParameterStore& UserParameters =
		AbsorptionSystem->GetExposedParameters();
	const FNiagaraVariable SourceMeshParameter(
		FNiagaraTypeDefinition(UNiagaraDataInterfaceSkeletalMesh::StaticClass()),
		TEXT("User.SourceMesh"));
	const FNiagaraVariable TargetPositionParameter(
		FNiagaraTypeDefinition::GetPositionDef(),
		TEXT("User.AbsorbTargetPosition"));
	const FNiagaraVariable StrengthParameter(
		FNiagaraTypeDefinition::GetFloatDef(),
		TEXT("User.AbsorbStrength"));
	const FNiagaraVariable RadiusParameter(
		FNiagaraTypeDefinition::GetFloatDef(),
		TEXT("User.AbsorbRadius"));
	const FNiagaraVariable KillRadiusParameter(
		FNiagaraTypeDefinition::GetFloatDef(),
		TEXT("User.AbsorbKillRadius"));

	// These exact User contracts are consumed by the native component before Niagara activation.
	TestTrue(TEXT("Source mesh data interface is exposed"), UserParameters.IndexOf(SourceMeshParameter) != INDEX_NONE);
	TestNotNull(TEXT("Source mesh data interface owns a default object"), UserParameters.GetDataInterface(SourceMeshParameter));
	TestTrue(TEXT("LWC absorption target Position is exposed"), UserParameters.IndexOf(TargetPositionParameter) != INDEX_NONE);
	TestTrue(TEXT("Runtime attraction strength is exposed"), UserParameters.IndexOf(StrengthParameter) != INDEX_NONE);
	TestTrue(TEXT("Runtime attraction radius is exposed"), UserParameters.IndexOf(RadiusParameter) != INDEX_NONE);
	TestTrue(TEXT("Arrival kill radius is exposed"), UserParameters.IndexOf(KillRadiusParameter) != INDEX_NONE);
	TestTrue(TEXT("Cross-actor GPU travel has authored fixed bounds"), AbsorptionSystem->bFixedBounds);
	TestTrue(
		TEXT("Authored fallback bounds exceed the original +/-100 cm sample"),
		AbsorptionSystem->GetFixedBounds().GetExtent().GetMin() >= 5000.0f);

	return !HasAnyErrors();
}

#endif
