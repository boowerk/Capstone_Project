#if WITH_DEV_AUTOMATION_TESTS

#include "Animation/AnimMontage.h"
#include "Animation/GP_AnimNotify_SendGameplayEvent.h"
#include "Animation/PDA_CharacterAnimationSet.h"
#include "Animation/PDA_EnemyAnimationSet.h"
#include "Characters/GP_EnemyCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Game/RegionEvents/GP_RegionEventData.h"
#include "GameplayTags/GP_Tags.h"
#include "Misc/AutomationTest.h"

namespace EnemyProductionAnimationTests
{
	const UPDA_EnemyAnimationSet* ResolveEnemyAnimationSet(const AGP_EnemyCharacter* Enemy)
	{
		if (!IsValid(Enemy))
		{
			return nullptr;
		}

		if (const UPDA_EnemyAnimationSet* ExplicitSet = Enemy->GetEnemyAnimationSet())
		{
			return ExplicitSet;
		}

		// Native basic enemies deliberately keep this package soft until runtime
		// initialization; tests resolve it after the editor has fully started.
		const TSoftObjectPtr<UPDA_EnemyAnimationSet>& DefaultSet = Enemy->GetDefaultEnemyAnimationSet();
		return DefaultSet.IsNull() ? nullptr : DefaultSet.LoadSynchronous();
	}

	// Resolve the same data paths as UGP_EnemyAttack so this contract catches
	// production classes that would otherwise fall back to an instant hit.
	UAnimMontage* ResolveFirstAttackMontage(const AGP_EnemyCharacter* Enemy)
	{
		if (!IsValid(Enemy))
		{
			return nullptr;
		}

		if (const UPDA_EnemyAnimationSet* EnemySet = ResolveEnemyAnimationSet(Enemy))
		{
			for (UAnimMontage* Montage : EnemySet->LightAttackMontages)
			{
				if (IsValid(Montage))
				{
					return Montage;
				}
			}
			return EnemySet->PrimaryAttackMontage.Get();
		}

		if (const UPDA_CharacterAnimationSet* LegacySet = Enemy->AnimationSet)
		{
			for (UAnimMontage* Montage : LegacySet->LightAttackMontages)
			{
				if (IsValid(Montage))
				{
					return Montage;
				}
			}
			return LegacySet->PrimaryAttackMontage.Get();
		}
		return nullptr;
	}

	bool HasGameplayEventNotify(const UAnimMontage* Montage, const FGameplayTag& EventTag)
	{
		if (!IsValid(Montage))
		{
			return false;
		}

		for (const FAnimNotifyEvent& NotifyEvent : Montage->Notifies)
		{
			const UGP_AnimNotify_SendGameplayEvent* GameplayEventNotify =
				Cast<UGP_AnimNotify_SendGameplayEvent>(NotifyEvent.Notify);
			if (IsValid(GameplayEventNotify)
				&& GameplayEventNotify->GameplayEventTag.MatchesTagExact(EventTag))
			{
				return true;
			}
		}
		return false;
	}

	bool AnimationSetHasGameplayEvent(
		const UPDA_EnemyAnimationSet* AnimationSet,
		const FGameplayTag& EventTag)
	{
		if (!IsValid(AnimationSet))
		{
			return false;
		}

		if (HasGameplayEventNotify(AnimationSet->PrimaryAttackMontage, EventTag))
		{
			return true;
		}
		for (const UAnimMontage* Montage : AnimationSet->LightAttackMontages)
		{
			if (HasGameplayEventNotify(Montage, EventTag))
			{
				return true;
			}
		}
		return false;
	}

	void ValidateEnemyAnimationContract(
		FAutomationTestBase& Test,
		const UClass* EnemyClass,
		const FString& Context)
	{
		const AGP_EnemyCharacter* Enemy = IsValid(EnemyClass)
			? Cast<AGP_EnemyCharacter>(EnemyClass->GetDefaultObject())
			: nullptr;
		Test.TestNotNull(FString::Printf(TEXT("%s enemy CDO loads"), *Context), Enemy);
		if (!IsValid(Enemy))
		{
			return;
		}

		const UPDA_EnemyAnimationSet* EnemySet = ResolveEnemyAnimationSet(Enemy);
		const UPDA_CharacterAnimationSet* LegacySet = Enemy->AnimationSet;
		const USkeletalMeshComponent* MeshComponent = Enemy->GetMesh();
		const USkeletalMesh* Mesh = IsValid(EnemySet) && IsValid(EnemySet->CharacterMesh)
			? EnemySet->CharacterMesh.Get()
			: (IsValid(MeshComponent) ? MeshComponent->GetSkeletalMeshAsset() : nullptr);
		Test.TestNotNull(FString::Printf(TEXT("%s has a skeletal mesh"), *Context), Mesh);

		Test.TestTrue(
			FString::Printf(TEXT("%s has an enemy or legacy animation set"), *Context),
			IsValid(EnemySet) || IsValid(LegacySet));

		const UClass* AnimBlueprintClass = IsValid(EnemySet)
			? EnemySet->AnimBlueprintClass.Get()
			: (IsValid(LegacySet) ? LegacySet->AnimBlueprintClass.Get() : nullptr);
		Test.TestTrue(
			FString::Printf(TEXT("%s has an animation blueprint class"), *Context),
			IsValid(AnimBlueprintClass));

		UAnimMontage* AttackMontage = ResolveFirstAttackMontage(Enemy);
		Test.TestNotNull(FString::Printf(TEXT("%s has a visible attack montage"), *Context), AttackMontage);
		if (IsValid(Mesh) && IsValid(AttackMontage))
		{
			Test.TestTrue(
				FString::Printf(TEXT("%s attack montage matches the mesh skeleton"), *Context),
				AttackMontage->GetSkeleton() == Mesh->GetSkeleton());
		}
		if (IsValid(EnemySet))
		{
			// GAS must receive authored frames; otherwise montage completion
			// falls back to a late, visually disconnected hit.
			Test.TestTrue(
				FString::Printf(TEXT("%s has an enemy attack-hit notify"), *Context),
				AnimationSetHasGameplayEvent(EnemySet, GPTags::Event::Enemy::AttackHit));
			Test.TestTrue(
				FString::Printf(TEXT("%s has an enemy action-end notify"), *Context),
				AnimationSetHasGameplayEvent(EnemySet, GPTags::Event::Enemy::ActionEnd));
		}
	}

	void ValidateBasicMeshPresentation(
		FAutomationTestBase& Test,
		UWorld* TestWorld,
		const UClass* EnemyClass,
		const FString& Context,
		float ExpectedRelativeZ,
		float ExpectedUniformScale)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AGP_EnemyCharacter* Enemy = IsValid(TestWorld) && IsValid(EnemyClass)
			? TestWorld->SpawnActor<AGP_EnemyCharacter>(
				const_cast<UClass*>(EnemyClass),
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				SpawnParameters)
			: nullptr;
		Test.TestNotNull(FString::Printf(TEXT("%s runtime actor spawns"), *Context), Enemy);
		if (!IsValid(Enemy))
		{
			return;
		}

		// A bare transient world does not advance the actor initialization
		// lifecycle, so invoke the same virtual hook used by PostInitializeComponents.
		Enemy->UpdateAnimationSet();
		const USkeletalMeshComponent* MeshComponent = IsValid(Enemy) ? Enemy->GetMesh() : nullptr;
		Test.TestNotNull(FString::Printf(TEXT("%s presentation mesh exists"), *Context), MeshComponent);
		if (!IsValid(MeshComponent))
		{
			return;
		}

		// These values mirror the proven production Blueprint using the same
		// mesh, keeping feet, silhouette size, capsule, and health bar aligned.
		Test.TestTrue(
			FString::Printf(TEXT("%s mesh ground offset matches production"), *Context),
			FMath::IsNearlyEqual(MeshComponent->GetRelativeLocation().Z, ExpectedRelativeZ));
		const FVector RelativeScale = MeshComponent->GetRelativeScale3D();
		Test.TestTrue(
			FString::Printf(TEXT("%s mesh scale matches production"), *Context),
			RelativeScale.Equals(FVector(ExpectedUniformScale), KINDA_SMALL_NUMBER));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEnemyProductionAnimationContractTest,
	"ProjectEden.AI.Enemy.ProductionAnimationContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyProductionAnimationContractTest::RunTest(const FString& Parameters)
{
	using namespace EnemyProductionAnimationTests;

	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Created production enemy presentation test world"), TestWorld))
	{
		return false;
	}

	struct FBasicEnemyPresentationCase
	{
		const TCHAR* ClassPath;
		float ExpectedRelativeZ;
		float ExpectedUniformScale;
	};

	const FBasicEnemyPresentationCase BasicEnemyCases[] =
	{
		{ TEXT("/Game/Characters/EnemyCharacter/Basic/BP_BasicEnemy_Melee.BP_BasicEnemy_Melee_C"), -89.0f, 1.9f },
		{ TEXT("/Game/Characters/EnemyCharacter/Basic/BP_BasicEnemy_Ranged.BP_BasicEnemy_Ranged_C"), -88.0f, 1.5425f },
		{ TEXT("/Game/Characters/EnemyCharacter/Basic/BP_BasicEnemy_Flying.BP_BasicEnemy_Flying_C"), -88.0f, 1.5425f }
	};
	for (const FBasicEnemyPresentationCase& BasicEnemyCase : BasicEnemyCases)
	{
		const UClass* EnemyClass = LoadClass<AGP_EnemyCharacter>(nullptr, BasicEnemyCase.ClassPath);
		ValidateEnemyAnimationContract(*this, EnemyClass, BasicEnemyCase.ClassPath);
		ValidateBasicMeshPresentation(
			*this,
			TestWorld,
			EnemyClass,
			BasicEnemyCase.ClassPath,
			BasicEnemyCase.ExpectedRelativeZ,
			BasicEnemyCase.ExpectedUniformScale);
	}

	const TCHAR* ProductionEventPaths[] =
	{
		TEXT("/Game/RegionEvents/Runtime/DA_RE_World_RedRift.DA_RE_World_RedRift"),
		TEXT("/Game/RegionEvents/Runtime/DA_RE_World_StructureDefense.DA_RE_World_StructureDefense")
	};
	for (const TCHAR* EventPath : ProductionEventPaths)
	{
		const UGP_RegionEventData* EventData = LoadObject<UGP_RegionEventData>(nullptr, EventPath);
		TestNotNull(FString::Printf(TEXT("Production event loads: %s"), EventPath), EventData);
		if (!IsValid(EventData))
		{
			continue;
		}

		for (const FGP_EnemySpawnEntry& SpawnEntry : EventData->EnemySpawns)
		{
			ValidateEnemyAnimationContract(
				*this,
				SpawnEntry.EnemyClass.Get(),
				FString::Printf(TEXT("%s spawn %s"), EventPath, *GetNameSafe(SpawnEntry.EnemyClass.Get())));
		}
	}

	TestWorld->DestroyWorld(false);
	return !HasAnyErrors();
}

#endif
