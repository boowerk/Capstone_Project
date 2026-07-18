#if WITH_DEV_AUTOMATION_TESTS

#include "Animation/AnimMontage.h"
#include "Animation/GP_AnimNotify_SendGameplayEvent.h"
#include "Animation/PDA_CharacterAnimationSet.h"
#include "Animation/PDA_EnemyAnimationSet.h"
#include "Characters/GP_EnemyCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEnemyProductionAnimationContractTest,
	"ProjectEden.AI.Enemy.ProductionAnimationContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyProductionAnimationContractTest::RunTest(const FString& Parameters)
{
	using namespace EnemyProductionAnimationTests;

	const TCHAR* BasicEnemyClassPaths[] =
	{
		TEXT("/Game/Characters/EnemyCharacter/Basic/BP_BasicEnemy_Melee.BP_BasicEnemy_Melee_C"),
		TEXT("/Game/Characters/EnemyCharacter/Basic/BP_BasicEnemy_Ranged.BP_BasicEnemy_Ranged_C"),
		TEXT("/Game/Characters/EnemyCharacter/Basic/BP_BasicEnemy_Flying.BP_BasicEnemy_Flying_C")
	};
	for (const TCHAR* ClassPath : BasicEnemyClassPaths)
	{
		const UClass* EnemyClass = LoadClass<AGP_EnemyCharacter>(nullptr, ClassPath);
		ValidateEnemyAnimationContract(*this, EnemyClass, ClassPath);
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

	return !HasAnyErrors();
}

#endif
