#include "Characters/GP_FlyingEnemyCharacter.h"
#include "Characters/GP_MeleeEnemyCharacter.h"
#include "Characters/GP_RangedEnemyCharacter.h"

#include "AI/Data/EnemyArchetypeData.h"
#include "AbilitySystem/Abilities/Enemy/GP_EnemyAttack.h"
#include "AbilitySystem/Abilities/Enemy/GP_EnemyRangedAttack.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTags/GP_Tags.h"

namespace GPBasicEnemyDefaults
{
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
	CombatArchetype = EGPEnemyCombatArchetype::Melee;
	ReturnHomeDistance = 2200.0f;
	ReturnHomeAcceptanceRadius = 150.0f;
	PatrolRadius = 900.0f;
	SightRadius = 1800.0f;
	LoseSightRadius = 2200.0f;
	PeripheralVisionAngleDegrees = 85.0f;
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

AGP_RangedEnemyCharacter::AGP_RangedEnemyCharacter()
{
	CombatArchetype = EGPEnemyCombatArchetype::Ranged;
	ReturnHomeDistance = 3000.0f;
	ReturnHomeAcceptanceRadius = 180.0f;
	PatrolRadius = 1400.0f;
	SightRadius = 2600.0f;
	LoseSightRadius = 3200.0f;
	PeripheralVisionAngleDegrees = 95.0f;
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

AGP_FlyingEnemyCharacter::AGP_FlyingEnemyCharacter()
{
	CombatArchetype = EGPEnemyCombatArchetype::Flying;
	ReturnHomeDistance = 3400.0f;
	ReturnHomeAcceptanceRadius = 220.0f;
	PatrolRadius = 1600.0f;
	SightRadius = 3000.0f;
	LoseSightRadius = 3800.0f;
	PeripheralVisionAngleDegrees = 120.0f;
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

void AGP_FlyingEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		// Force the runtime movement mode too, because placed Blueprint defaults can be edited independently of C++ constructor values.
		MovementComponent->SetMovementMode(MOVE_Flying);
	}
}
