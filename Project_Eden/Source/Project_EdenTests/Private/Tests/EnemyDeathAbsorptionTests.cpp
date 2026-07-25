#if WITH_DEV_AUTOMATION_TESTS

#include "Actors/GP_MatadorBossDecoyActor.h"
#include "Characters/GP_CrystalSeraphBossCharacter.h"
#include "Characters/GP_EnemyCharacter.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "Misc/AutomationTest.h"
#include "NiagaraDataInterfaceSkeletalMesh.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraSystem.h"
#include "VFX/GP_BossDeathPresentationComponent.h"
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
	TestTrue(TEXT("Real enemies enable shared death absorption by default"), ComponentDefaults->IsDeathAbsorptionEnabled());
	TestNotNull(TEXT("A safe default body dissolve material is configured"), ComponentDefaults->GetDefaultDeathDissolveMaterial());
	TestNotNull(TEXT("A safe default absorption-particle material is configured"), ComponentDefaults->GetDeathParticleMaterial());
	TestEqual(TEXT("Source mesh parameter contract"), ComponentDefaults->SourceMeshParameterName, FName(TEXT("User.SourceMesh")));
	TestEqual(
		TEXT("Target position parameter contract"),
		ComponentDefaults->AbsorbTargetPositionParameterName,
		FName(TEXT("User.AbsorbTargetPosition")));
	TestEqual(
		TEXT("Strength parameter contract"),
		ComponentDefaults->AbsorbStrengthParameterName,
		FName(TEXT("User.AbsorbStrength")));
	TestEqual(
		TEXT("Falling gravity parameter contract"),
		ComponentDefaults->FallGravityParameterName,
		FName(TEXT("User.FallGravity")));
	TestEqual(
		TEXT("Absorption drag parameter contract"),
		ComponentDefaults->AbsorbDragParameterName,
		FName(TEXT("User.AbsorbDrag")));
	TestTrue(
		TEXT("Five-second source curves are compressed inside the two-second corpse window"),
		ComponentDefaults->NiagaraPlaybackRate >= 2.5f
			&& ComponentDefaults->NiagaraPlaybackRate <= 2.7f
			&& ComponentDefaults->EffectDeactivateTimeSeconds <= 1.90f);
	TestTrue(
		TEXT("High-rate emission is stopped after a short authored burst"),
		ComponentDefaults->EmissionStopTimeSeconds * ComponentDefaults->NiagaraPlaybackRate <= 0.25f);
	TestTrue(
		TEXT("A first-frame hitch cannot stop emission before Niagara produces a visible sample"),
		ComponentDefaults->MinimumEmissionFrames >= 3);
	TestTrue(
		TEXT("The source material remains long enough for a visible dissolve transition"),
		ComponentDefaults->SourceMeshHideDelaySeconds >= 0.3f
			&& ComponentDefaults->SourceMeshHideDelaySeconds < ComponentDefaults->EffectDeactivateTimeSeconds);
	TestEqual(
		TEXT("Niagara material override contract"),
		ComponentDefaults->DeathParticleMaterialParameterName,
		FName(TEXT("User.DeathParticleMaterial")));
	TestEqual(
		TEXT("Body and fragment material progress contract"),
		ComponentDefaults->DissolveProgressParameterName,
		FName(TEXT("DissolveProgress")));

	const AGP_CrystalSeraphBossCharacter* CrystalDefaults =
		GetDefault<AGP_CrystalSeraphBossCharacter>();
	const UGP_EnemyDeathAbsorptionComponent* CrystalAbsorption =
		CrystalDefaults ? CrystalDefaults->GetEnemyDeathAbsorptionComponent() : nullptr;
	TestTrue(
		TEXT("Bosses participate in the same shared death absorption flow"),
		IsValid(CrystalAbsorption) && CrystalAbsorption->IsDeathAbsorptionEnabled());

	const AGP_MatadorBossDecoyActor* DecoyDefaults =
		GetDefault<AGP_MatadorBossDecoyActor>();
	const UGP_EnemyDeathAbsorptionComponent* DecoyAbsorption =
		DecoyDefaults ? DecoyDefaults->GetEnemyDeathAbsorptionComponent() : nullptr;
	TestTrue(
		TEXT("Matador decoys keep their bespoke break presentation"),
		IsValid(DecoyAbsorption) && !DecoyAbsorption->IsDeathAbsorptionEnabled());

	const float MaximumStrength = 800.0f;
	TestEqual(
		TEXT("Falling phase has no attraction"),
		UGP_EnemyDeathAbsorptionComponent::CalculateAbsorbStrength(0.38f, 0.38f, 0.80f, MaximumStrength),
		0.0f);
	TestTrue(
		TEXT("Attraction ramps while gravity fades"),
		FMath::IsNearlyEqual(
			UGP_EnemyDeathAbsorptionComponent::CalculateAbsorbStrength(0.78f, 0.38f, 0.80f, MaximumStrength),
			MaximumStrength * 0.5f,
			0.1f));
	TestEqual(
		TEXT("Attraction reaches its configured maximum"),
		UGP_EnemyDeathAbsorptionComponent::CalculateAbsorbStrength(1.18f, 0.38f, 0.80f, MaximumStrength),
		MaximumStrength);
	TestEqual(
		TEXT("Falling gravity starts fully enabled"),
		UGP_EnemyDeathAbsorptionComponent::CalculateFallGravityScale(0.0f, 0.28f, 0.60f),
		1.0f);
	TestEqual(
		TEXT("Falling gravity remains full through its hold"),
		UGP_EnemyDeathAbsorptionComponent::CalculateFallGravityScale(0.28f, 0.28f, 0.60f),
		1.0f);
	TestTrue(
		TEXT("Falling gravity is half strength midway through its smooth fade"),
		FMath::IsNearlyEqual(
			UGP_EnemyDeathAbsorptionComponent::CalculateFallGravityScale(0.44f, 0.28f, 0.60f),
			0.5f,
			0.001f));
	TestEqual(
		TEXT("Falling gravity is disabled before the final attraction"),
		UGP_EnemyDeathAbsorptionComponent::CalculateFallGravityScale(0.60f, 0.28f, 0.60f),
		0.0f);

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
	const FNiagaraVariable FallGravityParameter(
		FNiagaraTypeDefinition::GetVec3Def(),
		TEXT("User.FallGravity"));
	const FNiagaraVariable AbsorbDragParameter(
		FNiagaraTypeDefinition::GetFloatDef(),
		TEXT("User.AbsorbDrag"));
	const FNiagaraVariable SpriteSizeParameter(
		FNiagaraTypeDefinition::GetVec2Def(),
		TEXT("User.SpriteSize"));
	const FNiagaraVariable DeathParticleMaterialParameter(
		FNiagaraTypeDefinition::GetUMaterialDef(),
		TEXT("User.DeathParticleMaterial"));

	// These exact User contracts are consumed by the native component before Niagara activation.
	TestTrue(TEXT("Source mesh data interface is exposed"), UserParameters.IndexOf(SourceMeshParameter) != INDEX_NONE);
	TestNotNull(TEXT("Source mesh data interface owns a default object"), UserParameters.GetDataInterface(SourceMeshParameter));
	TestTrue(TEXT("LWC absorption target Position is exposed"), UserParameters.IndexOf(TargetPositionParameter) != INDEX_NONE);
	TestTrue(TEXT("Runtime attraction strength is exposed"), UserParameters.IndexOf(StrengthParameter) != INDEX_NONE);
	TestTrue(TEXT("Runtime attraction radius is exposed"), UserParameters.IndexOf(RadiusParameter) != INDEX_NONE);
	TestTrue(TEXT("Arrival kill radius is exposed"), UserParameters.IndexOf(KillRadiusParameter) != INDEX_NONE);
	TestTrue(TEXT("Runtime falling gravity is exposed"), UserParameters.IndexOf(FallGravityParameter) != INDEX_NONE);
	TestTrue(TEXT("Runtime absorption drag is exposed"), UserParameters.IndexOf(AbsorbDragParameter) != INDEX_NONE);
	TestTrue(TEXT("Visible grain size remains exposed"), UserParameters.IndexOf(SpriteSizeParameter) != INDEX_NONE);
	TestTrue(
		TEXT("Per-enemy particle material is exposed"),
		UserParameters.IndexOf(DeathParticleMaterialParameter) != INDEX_NONE);
	TestTrue(
		TEXT("Designer-tuned grains retain their visible 10x10 size"),
		UserParameters.GetParameterValue<FVector2f>(SpriteSizeParameter).Equals(
			FVector2f(10.0f, 10.0f),
			KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Cross-actor GPU travel has authored fixed bounds"), AbsorptionSystem->bFixedBounds);
	TestTrue(
		TEXT("Authored fallback bounds exceed the original +/-100 cm sample"),
		AbsorptionSystem->GetFixedBounds().GetExtent().GetMin() >= 5000.0f);

	bool bFoundBoundSpriteRenderer = false;
	for (const FNiagaraEmitterHandle& EmitterHandle : AbsorptionSystem->GetEmitterHandles())
	{
		const FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData();
		if (!EmitterData)
		{
			continue;
		}

		for (const UNiagaraRendererProperties* RendererProperties : EmitterData->GetRenderers())
		{
			const UNiagaraSpriteRendererProperties* SpriteRenderer =
				Cast<UNiagaraSpriteRendererProperties>(RendererProperties);
			if (IsValid(SpriteRenderer)
				&& SpriteRenderer->MaterialUserParamBinding.Parameter == DeathParticleMaterialParameter)
			{
				bFoundBoundSpriteRenderer = true;
				break;
			}
		}
	}
	TestTrue(
		TEXT("Sprite Renderer consumes User.DeathParticleMaterial"),
		bFoundBoundSpriteRenderer);

	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEnemyDeathMaterialAssetContractTest,
	"ProjectEden.VFX.EnemyDeathAbsorption.MaterialAssetContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyDeathMaterialAssetContractTest::RunTest(const FString& Parameters)
{
	UMaterialInterface* BodyMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Game/Niagara/Dissolve_SK/EnemyMaterials/M_EnemyDeath_Dissolve.M_EnemyDeath_Dissolve"));
	UMaterialInterface* ParticleMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Game/Niagara/Dissolve_SK/EnemyMaterials/M_EnemyDeath_AbsorbParticle.M_EnemyDeath_AbsorbParticle"));

	TestNotNull(TEXT("Shared body dissolve material loads"), BodyMaterial);
	TestNotNull(TEXT("Shared absorption particle material loads"), ParticleMaterial);
	if (!IsValid(BodyMaterial) || !IsValid(ParticleMaterial))
	{
		return false;
	}

	auto HasScalarParameter = [](const UMaterialInterface* Material, const FName ParameterName)
	{
		TArray<FMaterialParameterInfo> ParameterInfos;
		TArray<FGuid> ParameterIds;
		Material->GetAllScalarParameterInfo(ParameterInfos, ParameterIds);
		return ParameterInfos.ContainsByPredicate(
			[ParameterName](const FMaterialParameterInfo& Info)
			{
				return Info.Name == ParameterName;
			});
	};
	auto HasVectorParameter = [](const UMaterialInterface* Material, const FName ParameterName)
	{
		TArray<FMaterialParameterInfo> ParameterInfos;
		TArray<FGuid> ParameterIds;
		Material->GetAllVectorParameterInfo(ParameterInfos, ParameterIds);
		return ParameterInfos.ContainsByPredicate(
			[ParameterName](const FMaterialParameterInfo& Info)
			{
				return Info.Name == ParameterName;
			});
	};

	TestTrue(
		TEXT("Body material exposes DissolveProgress"),
		HasScalarParameter(BodyMaterial, TEXT("DissolveProgress")));
	TestTrue(
		TEXT("Body material exposes EdgeWidth"),
		HasScalarParameter(BodyMaterial, TEXT("EdgeWidth")));
	TestTrue(
		TEXT("Body material exposes DeathTint"),
		HasVectorParameter(BodyMaterial, TEXT("DeathTint")));
	TestTrue(
		TEXT("Body material exposes EdgeColor"),
		HasVectorParameter(BodyMaterial, TEXT("EdgeColor")));
	TestTrue(
		TEXT("Particle material exposes DeathTint"),
		HasVectorParameter(ParticleMaterial, TEXT("DeathTint")));
	TestTrue(
		TEXT("Particle material exposes brightness"),
		HasScalarParameter(ParticleMaterial, TEXT("ParticleBrightness")));

	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEnemyDeathProductionMaterialConfigurationTest,
	"ProjectEden.VFX.EnemyDeathAbsorption.ProductionMaterialConfiguration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyDeathProductionMaterialConfigurationTest::RunTest(const FString& Parameters)
{
	struct FProductionEnemyExpectation
	{
		const TCHAR* GeneratedClassPath;
		const TCHAR* MaterialLabel;
		bool bBoss;
	};

	const FProductionEnemyExpectation Expectations[] =
	{
		{
			TEXT("/Game/Characters/EnemyCharacter/Monsters/FurnaceWalker/BP_FurnaceWalker.BP_FurnaceWalker_C"),
			TEXT("FurnaceWalker"),
			false,
		},
		{
			TEXT("/Game/Characters/EnemyCharacter/Monsters/FurnaceStomper/BP_FurnaceStomper.BP_FurnaceStomper_C"),
			TEXT("FurnaceStomper"),
			false,
		},
		{
			TEXT("/Game/Characters/EnemyCharacter/Monsters/CyclopsSpecter/BP_CyclopsSpecter.BP_CyclopsSpecter_C"),
			TEXT("CyclopsSpecter"),
			false,
		},
		{
			TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_CrystalSeraph/BP_Crystal_Seraph.BP_Crystal_Seraph_C"),
			TEXT("CrystalSeraph"),
			true,
		},
		{
			TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_DarkArmorKnight/BP_DarkArmorKnight.BP_DarkArmorKnight_C"),
			TEXT("DarkArmorKnight"),
			true,
		},
		{
			TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_Matador/BP_Boss_Matador.BP_Boss_Matador_C"),
			TEXT("Matador"),
			true,
		},
		{
			TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_Sans/BP_Boss_Sans.BP_Boss_Sans_C"),
			TEXT("Sans"),
			true,
		},
	};

	for (const FProductionEnemyExpectation& Expectation : Expectations)
	{
		UClass* EnemyClass = LoadObject<UClass>(nullptr, Expectation.GeneratedClassPath);
		if (!TestNotNull(
			FString::Printf(TEXT("Production enemy class loads: %s"), Expectation.GeneratedClassPath),
			EnemyClass))
		{
			continue;
		}

		const AGP_EnemyCharacter* EnemyDefaults =
			Cast<AGP_EnemyCharacter>(EnemyClass->GetDefaultObject());
		const UGP_EnemyDeathAbsorptionComponent* AbsorptionComponent =
			IsValid(EnemyDefaults)
				? EnemyDefaults->GetEnemyDeathAbsorptionComponent()
				: nullptr;
		if (!TestNotNull(
			FString::Printf(TEXT("%s owns death absorption"), Expectation.MaterialLabel),
			AbsorptionComponent))
		{
			continue;
		}

		const UMaterialInterface* BodyMaterial =
			AbsorptionComponent->GetDefaultDeathDissolveMaterial();
		const UMaterialInterface* ParticleMaterial =
			AbsorptionComponent->GetDeathParticleMaterial();
		TestTrue(
			FString::Printf(TEXT("%s has its auxiliary body material"), Expectation.MaterialLabel),
			IsValid(BodyMaterial)
				&& BodyMaterial->GetPathName().Contains(Expectation.MaterialLabel)
				&& BodyMaterial->GetPathName().Contains(TEXT("_Auxiliary")));
		TestTrue(
			FString::Printf(TEXT("%s has its particle material"), Expectation.MaterialLabel),
			IsValid(ParticleMaterial)
				&& ParticleMaterial->GetPathName().Contains(Expectation.MaterialLabel));
		TestTrue(
			FString::Printf(TEXT("%s has slot material overrides"), Expectation.MaterialLabel),
			!AbsorptionComponent->GetDeathDissolveMaterialOverrides().IsEmpty());

		if (Expectation.bBoss)
		{
			const UGP_BossDeathPresentationComponent* PresentationComponent =
				EnemyDefaults->GetBossDeathPresentationComponent();
			TestTrue(
				FString::Printf(TEXT("%s has a fragment material"), Expectation.MaterialLabel),
				IsValid(PresentationComponent)
					&& IsValid(PresentationComponent->GetFragmentMaterial())
					&& PresentationComponent->GetFragmentMaterial()->GetPathName().Contains(
						Expectation.MaterialLabel));
			TestTrue(
				FString::Printf(TEXT("%s leaves source hide to absorption"), Expectation.MaterialLabel),
				IsValid(PresentationComponent)
					&& !PresentationComponent->DoesPresentationHideSourceMesh());
		}
	}

	return !HasAnyErrors();
}

#endif
