#if WITH_DEV_AUTOMATION_TESTS

#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
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

	// Resolve the same candidate pool as UGP_EnemyAttack. The primary montage is
	// selectable only when the light-attack array has no valid entries.
	TArray<const UAnimMontage*> ResolveSelectableAttackMontages(const AGP_EnemyCharacter* Enemy)
	{
		TArray<const UAnimMontage*> Montages;
		if (!IsValid(Enemy))
		{
			return Montages;
		}

		if (const UPDA_EnemyAnimationSet* EnemySet = ResolveEnemyAnimationSet(Enemy))
		{
			for (const UAnimMontage* Montage : EnemySet->LightAttackMontages)
			{
				if (IsValid(Montage))
				{
					Montages.AddUnique(Montage);
				}
			}
			if (Montages.IsEmpty() && IsValid(EnemySet->PrimaryAttackMontage))
			{
				Montages.Add(EnemySet->PrimaryAttackMontage);
			}
			return Montages;
		}

		if (const UPDA_CharacterAnimationSet* LegacySet = Enemy->AnimationSet)
		{
			for (const UAnimMontage* Montage : LegacySet->LightAttackMontages)
			{
				if (IsValid(Montage))
				{
					Montages.AddUnique(Montage);
				}
			}
			if (Montages.IsEmpty() && IsValid(LegacySet->PrimaryAttackMontage))
			{
				Montages.Add(LegacySet->PrimaryAttackMontage);
			}
		}
		return Montages;
	}

	struct FAttackNotifyContract
	{
		int32 AttackHitCount = 0;
		int32 ActionEndCount = 0;
		float AttackHitTime = 0.0f;
		float ActionEndTime = 0.0f;
	};

	FAttackNotifyContract InspectAttackNotifyContract(const UAnimMontage* Montage)
	{
		FAttackNotifyContract Contract;
		if (!IsValid(Montage))
		{
			return Contract;
		}

		// Count exact gameplay-event notifies on each runtime-selectable montage.
		for (const FAnimNotifyEvent& NotifyEvent : Montage->Notifies)
		{
			const UGP_AnimNotify_SendGameplayEvent* GameplayEventNotify =
				Cast<UGP_AnimNotify_SendGameplayEvent>(NotifyEvent.Notify);
			if (!IsValid(GameplayEventNotify))
			{
				continue;
			}

			if (GameplayEventNotify->GameplayEventTag.MatchesTagExact(GPTags::Event::Enemy::AttackHit))
			{
				++Contract.AttackHitCount;
				Contract.AttackHitTime = NotifyEvent.GetTriggerTime();
			}
			else if (GameplayEventNotify->GameplayEventTag.MatchesTagExact(GPTags::Event::Enemy::ActionEnd))
			{
				++Contract.ActionEndCount;
				Contract.ActionEndTime = NotifyEvent.GetTriggerTime();
			}
		}
		return Contract;
	}

	void ValidateSelectableAttackMontage(
		FAutomationTestBase& Test,
		const UAnimMontage* Montage,
		const USkeletalMesh* Mesh,
		const FString& Context,
		bool bRequireGameplayEventContract)
	{
		Test.TestNotNull(FString::Printf(TEXT("%s montage loads"), *Context), Montage);
		if (!IsValid(Montage))
		{
			return;
		}

		if (IsValid(Mesh))
		{
			Test.TestTrue(
				FString::Printf(TEXT("%s matches the mesh skeleton"), *Context),
				Montage->GetSkeleton() == Mesh->GetSkeleton());
		}

		if (!bRequireGameplayEventContract)
		{
			return;
		}

		const FAttackNotifyContract Contract = InspectAttackNotifyContract(Montage);
		Test.TestEqual(
			FString::Printf(TEXT("%s has exactly one AttackHit notify"), *Context),
			Contract.AttackHitCount,
			1);
		Test.TestEqual(
			FString::Printf(TEXT("%s has exactly one ActionEnd notify"), *Context),
			Contract.ActionEndCount,
			1);

		if (Contract.AttackHitCount == 1 && Contract.ActionEndCount == 1)
		{
			const float PlayLength = Montage->GetPlayLength();
			const FString TimingContext = FString::Printf(
				TEXT("%s (Hit=%.3fs, End=%.3fs, Length=%.3fs)"),
				*Context,
				Contract.AttackHitTime,
				Contract.ActionEndTime,
				PlayLength);
			Test.TestTrue(
				FString::Printf(TEXT("%s AttackHit occurs before ActionEnd"), *TimingContext),
				Contract.AttackHitTime < Contract.ActionEndTime);
			Test.TestTrue(
				FString::Printf(TEXT("%s ActionEnd stays inside montage playback"), *TimingContext),
				Contract.ActionEndTime <= PlayLength + KINDA_SMALL_NUMBER);
		}
	}

	void ValidateCombatTransitionAnimation(
		FAutomationTestBase& Test,
		const UAnimSequence* TransitionAnimation,
		const USkeletalMesh* Mesh,
		const FString& Context,
		float DurationSeconds)
	{
		Test.TestNotNull(FString::Printf(TEXT("%s resolves a transition animation"), *Context), TransitionAnimation);
		if (!IsValid(TransitionAnimation))
		{
			return;
		}

		if (IsValid(Mesh))
		{
			// Never let the temporary fallback cross enemy skeleton families.
			Test.TestTrue(
				FString::Printf(TEXT("%s matches the enemy skeleton"), *Context),
				TransitionAnimation->GetSkeleton() == Mesh->GetSkeleton());
		}
		Test.TestTrue(
			FString::Printf(TEXT("%s has playable source frames"), *Context),
			TransitionAnimation->GetPlayLength() > KINDA_SMALL_NUMBER);
		Test.TestTrue(
			FString::Printf(TEXT("%s has a positive bridge duration"), *Context),
			DurationSeconds > KINDA_SMALL_NUMBER);
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

		if (IsValid(EnemySet))
		{
			Test.TestFalse(
				FString::Printf(TEXT("%s has a transition montage slot"), *Context),
				EnemySet->CombatTransitionSlotName.IsNone());
			ValidateCombatTransitionAnimation(
				Test,
				EnemySet->ResolveAttackPrepareAnimation(),
				Mesh,
				FString::Printf(TEXT("%s / AttackPrepare"), *Context),
				EnemySet->AttackPrepareDurationSeconds);
			ValidateCombatTransitionAnimation(
				Test,
				EnemySet->ResolveChaseResumeAnimation(),
				Mesh,
				FString::Printf(TEXT("%s / ChaseResume"), *Context),
				EnemySet->ChaseResumeDurationSeconds);
		}

		const TArray<const UAnimMontage*> AttackMontages = ResolveSelectableAttackMontages(Enemy);
		Test.TestTrue(
			FString::Printf(TEXT("%s has at least one runtime-selectable attack montage"), *Context),
			!AttackMontages.IsEmpty());
		for (const UAnimMontage* AttackMontage : AttackMontages)
		{
			ValidateSelectableAttackMontage(
				Test,
				AttackMontage,
				Mesh,
				FString::Printf(TEXT("%s / %s"), *Context, *GetNameSafe(AttackMontage)),
				IsValid(EnemySet));
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
