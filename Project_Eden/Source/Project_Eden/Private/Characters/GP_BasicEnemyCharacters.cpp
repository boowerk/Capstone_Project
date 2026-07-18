#include "Characters/GP_FlyingEnemyCharacter.h"
#include "Characters/GP_MeleeEnemyCharacter.h"
#include "Characters/GP_RangedEnemyCharacter.h"

#include "AI/Data/EnemyArchetypeData.h"
#include "AbilitySystem/Abilities/Enemy/GP_EnemyAttack.h"
#include "AbilitySystem/Abilities/Enemy/GP_EnemyRangedAttack.h"
#include "Animation/PDA_EnemyAnimationSet.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTags/GP_Tags.h"
#include "UObject/ConstructorHelpers.h"

namespace GPBasicEnemyDefaults
{
	struct FSharedTemplateAssets
	{
		TObjectPtr<UBehaviorTree> CommonBehaviorTree = nullptr;
		TObjectPtr<UBlackboardData> CommonBlackboard = nullptr;
		TObjectPtr<USkeletalMesh> MaskManMesh = nullptr;
	};

	const FSharedTemplateAssets& GetSharedTemplateAssets()
	{
		static FSharedTemplateAssets SharedAssets;
		static bool bLoadedAssets = false;
		if (!bLoadedAssets)
		{
			// Basic Blueprint children should be usable immediately after creation, without hand-wiring common AI assets.
			static ConstructorHelpers::FObjectFinder<UBehaviorTree> CommonBehaviorTreeFinder(TEXT("/Game/Characters/EnemyCharacter/BT/Common/BT_EnemyCommon.BT_EnemyCommon"));
			if (CommonBehaviorTreeFinder.Succeeded())
			{
				SharedAssets.CommonBehaviorTree = CommonBehaviorTreeFinder.Object;
			}

			static ConstructorHelpers::FObjectFinder<UBlackboardData> CommonBlackboardFinder(TEXT("/Game/Characters/EnemyCharacter/BT/Common/BB_EnemyCommon.BB_EnemyCommon"));
			if (CommonBlackboardFinder.Succeeded())
			{
				SharedAssets.CommonBlackboard = CommonBlackboardFinder.Object;
			}

			static ConstructorHelpers::FObjectFinder<USkeletalMesh> MaskManMeshFinder(TEXT("/Game/Characters/MaskMan/SK_MaskMan.SK_MaskMan"));
			if (MaskManMeshFinder.Succeeded())
			{
				SharedAssets.MaskManMesh = MaskManMeshFinder.Object;
			}

			bLoadedAssets = true;
		}

		return SharedAssets;
	}

	void ConfigureTemplateMeshPresentation(
		USkeletalMeshComponent* MeshComponent,
		USkeletalMesh* FallbackMesh,
		float RelativeZ,
		float UniformScale)
	{
		if (!IsValid(MeshComponent))
		{
			return;
		}

		if (IsValid(FallbackMesh))
		{
			MeshComponent->SetSkeletalMesh(FallbackMesh);
		}

		// Enemy animation sets supply the runtime mesh and AnimBP, while the
		// component transform must mirror the corresponding production enemy.
		MeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, RelativeZ));
		MeshComponent->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
		MeshComponent->SetRelativeScale3D(FVector(UniformScale));
	}

	FEnemyArchetypeTuning MakeTuning(
		EEnemyMode Mode,
		float Aggression,
		float PreferredRange,
		float RetreatThreshold,
		float ChasePersistence,
		float CoverPreference,
		EEnemyFocusTargetRule FocusTargetRule)
	{
		FEnemyArchetypeTuning Tuning;
		Tuning.BaseEvaluation.EnemyMode = Mode;
		Tuning.BaseEvaluation.Aggression = Aggression;
		Tuning.BaseEvaluation.PreferredRange = PreferredRange;
		Tuning.BaseEvaluation.RetreatThreshold = RetreatThreshold;
		Tuning.BaseEvaluation.ChasePersistence = ChasePersistence;
		Tuning.BaseEvaluation.CoverPreference = CoverPreference;
		Tuning.BaseEvaluation.FocusTargetRule = FocusTargetRule;
		Tuning.BaseEvaluation.ValidateAndClamp();
		return Tuning;
	}
}

AGP_MeleeEnemyCharacter::AGP_MeleeEnemyCharacter()
{
	const GPBasicEnemyDefaults::FSharedTemplateAssets& SharedAssets = GPBasicEnemyDefaults::GetSharedTemplateAssets();
	BehaviorTreeAssetOverride = SharedAssets.CommonBehaviorTree;
	BlackboardAssetOverride = SharedAssets.CommonBlackboard;
	// Reuse the production Furnace Walker contract so default melee spawns have
	// locomotion, enemy hit notifies, and a visible attack wind-up.
	DefaultEnemyAnimationSet = TSoftObjectPtr<UPDA_EnemyAnimationSet>(FSoftObjectPath(
		TEXT("/Game/Characters/EnemyCharacter/Monsters/FurnaceWalker/PDA_FW_EnemyAnimationSet.PDA_FW_EnemyAnimationSet")));
	GPBasicEnemyDefaults::ConfigureTemplateMeshPresentation(
		GetMesh(), SharedAssets.MaskManMesh, -89.0f, 1.9f);

	CombatArchetype = EGPEnemyCombatArchetype::Melee;
	// Melee enemies recover quickly, but still vary enough that a surrounding group does not swing in unison.
	AttackCadenceSettings.InitialDelayMinSeconds = 0.05f;
	AttackCadenceSettings.InitialDelayMaxSeconds = 0.35f;
	AttackCadenceSettings.NextAttackDelayMinSeconds = 0.95f;
	AttackCadenceSettings.NextAttackDelayMaxSeconds = 1.45f;
	ReturnHomeDistance = 2200.0f;
	ReturnHomeAcceptanceRadius = 150.0f;
	PatrolRadius = 900.0f;
	SightRadius = 1800.0f;
	LoseSightRadius = 2200.0f;
	PeripheralVisionAngleDegrees = 85.0f;
	HearingRange = 1500.0f;
	XPReward = 20.0f;

	DefaultEnemyAttackAbilityClass = UGP_EnemyAttack::StaticClass();
	DefaultAttackAbilityTag = GPTags::Ability::Enemy::Attack_Melee;
	bUseBuiltInArchetypeTuning = true;
	BuiltInArchetypeTuning = GPBasicEnemyDefaults::MakeTuning(
		EEnemyMode::Pressure,
		0.35f,
		120.0f,
		0.2f,
		0.75f,
		0.1f,
		EEnemyFocusTargetRule::Nearest);

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->MaxWalkSpeed = 420.0f;
		MovementComponent->bOrientRotationToMovement = true;
	}
}

void AGP_MeleeEnemyCharacter::UpdateAnimationSet()
{
	Super::UpdateAnimationSet();

	// Blueprint children can retain an older serialized component offset, so
	// reapply the Furnace Walker presentation after resolving the runtime set.
	GPBasicEnemyDefaults::ConfigureTemplateMeshPresentation(GetMesh(), nullptr, -89.0f, 1.9f);
}

AGP_RangedEnemyCharacter::AGP_RangedEnemyCharacter()
{
	const GPBasicEnemyDefaults::FSharedTemplateAssets& SharedAssets = GPBasicEnemyDefaults::GetSharedTemplateAssets();
	BehaviorTreeAssetOverride = SharedAssets.CommonBehaviorTree;
	BlackboardAssetOverride = SharedAssets.CommonBlackboard;
	// Cyclops Specter already provides a ranged enemy AnimBP and enemy-tagged
	// attack montage, avoiding the retarget-only MaskMan player graph.
	DefaultEnemyAnimationSet = TSoftObjectPtr<UPDA_EnemyAnimationSet>(FSoftObjectPath(
		TEXT("/Game/Characters/EnemyCharacter/Monsters/CyclopsSpecter/PDA_CS_EnemyAnimationSet.PDA_CS_EnemyAnimationSet")));
	GPBasicEnemyDefaults::ConfigureTemplateMeshPresentation(
		GetMesh(), SharedAssets.MaskManMesh, -88.0f, 1.5425f);

	CombatArchetype = EGPEnemyCombatArchetype::Ranged;
	// Ranged attacks leave a wider breathing window after each projectile than melee attacks.
	AttackCadenceSettings.InitialDelayMinSeconds = 0.20f;
	AttackCadenceSettings.InitialDelayMaxSeconds = 0.70f;
	AttackCadenceSettings.NextAttackDelayMinSeconds = 1.45f;
	AttackCadenceSettings.NextAttackDelayMaxSeconds = 2.15f;
	ReturnHomeDistance = 3000.0f;
	ReturnHomeAcceptanceRadius = 180.0f;
	PatrolRadius = 1400.0f;
	SightRadius = 2600.0f;
	LoseSightRadius = 3200.0f;
	PeripheralVisionAngleDegrees = 95.0f;
	HearingRange = 1900.0f;
	XPReward = 30.0f;

	DefaultEnemyAttackAbilityClass = UGP_EnemyRangedAttack::StaticClass();
	DefaultAttackAbilityTag = GPTags::Ability::Enemy::Attack_Ranged;
	bUseBuiltInArchetypeTuning = true;
	BuiltInArchetypeTuning = GPBasicEnemyDefaults::MakeTuning(
		EEnemyMode::Hold,
		0.45f,
		850.0f,
		0.28f,
		0.65f,
		0.45f,
		EEnemyFocusTargetRule::PlayerFirst);

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->MaxWalkSpeed = 330.0f;
		MovementComponent->bOrientRotationToMovement = true;
	}
}

void AGP_RangedEnemyCharacter::UpdateAnimationSet()
{
	Super::UpdateAnimationSet();

	// Keep every generic ranged spawn aligned to the production Cyclops mesh.
	GPBasicEnemyDefaults::ConfigureTemplateMeshPresentation(GetMesh(), nullptr, -88.0f, 1.5425f);
}

AGP_FlyingEnemyCharacter::AGP_FlyingEnemyCharacter()
{
	const GPBasicEnemyDefaults::FSharedTemplateAssets& SharedAssets = GPBasicEnemyDefaults::GetSharedTemplateAssets();
	BehaviorTreeAssetOverride = SharedAssets.CommonBehaviorTree;
	BlackboardAssetOverride = SharedAssets.CommonBlackboard;
	// The spectral presentation tolerates flying movement and keeps the same
	// authoritative ranged timing contract as the grounded archetype.
	DefaultEnemyAnimationSet = TSoftObjectPtr<UPDA_EnemyAnimationSet>(FSoftObjectPath(
		TEXT("/Game/Characters/EnemyCharacter/Monsters/CyclopsSpecter/PDA_CS_EnemyAnimationSet.PDA_CS_EnemyAnimationSet")));
	GPBasicEnemyDefaults::ConfigureTemplateMeshPresentation(
		GetMesh(), SharedAssets.MaskManMesh, -88.0f, 1.5425f);

	CombatArchetype = EGPEnemyCombatArchetype::Flying;
	// Flying enemies use short, irregular bursts to distinguish them from grounded ranged enemies.
	AttackCadenceSettings.InitialDelayMinSeconds = 0.10f;
	AttackCadenceSettings.InitialDelayMaxSeconds = 0.55f;
	AttackCadenceSettings.NextAttackDelayMinSeconds = 0.80f;
	AttackCadenceSettings.NextAttackDelayMaxSeconds = 1.30f;
	ReturnHomeDistance = 3400.0f;
	ReturnHomeAcceptanceRadius = 220.0f;
	PatrolRadius = 1600.0f;
	SightRadius = 3000.0f;
	LoseSightRadius = 3800.0f;
	PeripheralVisionAngleDegrees = 120.0f;
	HearingRange = 2200.0f;
	XPReward = 35.0f;

	DefaultEnemyAttackAbilityClass = UGP_EnemyRangedAttack::StaticClass();
	DefaultAttackAbilityTag = GPTags::Ability::Enemy::Attack_Ranged;
	bUseBuiltInArchetypeTuning = true;
	BuiltInArchetypeTuning = GPBasicEnemyDefaults::MakeTuning(
		EEnemyMode::Pressure,
		0.6f,
		1000.0f,
		0.22f,
		0.8f,
		0.15f,
		EEnemyFocusTargetRule::PlayerFirst);

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->GravityScale = 0.0f;
		MovementComponent->MaxFlySpeed = 460.0f;
		MovementComponent->MaxWalkSpeed = 360.0f;
		MovementComponent->DefaultLandMovementMode = MOVE_Flying;
		MovementComponent->DefaultWaterMovementMode = MOVE_Flying;
		MovementComponent->bOrientRotationToMovement = true;
	}
}

void AGP_FlyingEnemyCharacter::UpdateAnimationSet()
{
	Super::UpdateAnimationSet();

	// Flying reuses the same spectral mesh contract until its dedicated P1
	// locomotion set is authored, including the proven component transform.
	GPBasicEnemyDefaults::ConfigureTemplateMeshPresentation(GetMesh(), nullptr, -88.0f, 1.5425f);
}

void AGP_FlyingEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		// Force the runtime movement mode too, because placed Blueprint defaults can be edited independently of C++ constructor values.
		MovementComponent->SetMovementMode(MOVE_Flying);
	}
}
