#include "AbilitySystem/Abilities/Enemy/GP_EnemyRangedAttack.h"

#include "AIController.h"
#include "AI/Data/EnemyBlackboardKeys.h"
#include "Actors/GP_EnemyRangedProjectile.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameplayTags/GP_Tags.h"

UGP_EnemyRangedAttack::UGP_EnemyRangedAttack()
{
	FGameplayTagContainer AbilityAssetTags;
	AbilityAssetTags.AddTag(GPTags::Ability::Enemy::Attack_Ranged);
	SetAssetTags(AbilityAssetTags);

	ProjectileClass = AGP_EnemyRangedProjectile::StaticClass();
}

void UGP_EnemyRangedAttack::PerformAttackHit()
{
	if (!HasAuthority(&CurrentActivationInfo))
	{
		return;
	}

	APawn* AvatarPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (!IsValid(AvatarPawn) || !*ProjectileClass || AvatarPawn->GetWorld() == nullptr)
	{
		return;
	}

	AActor* TargetActor = nullptr;
	if (const AAIController* AIController = Cast<AAIController>(AvatarPawn->GetController()))
	{
		if (const UBlackboardComponent* BlackboardComponent = AIController->GetBlackboardComponent())
		{
			TargetActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(EnemyBlackboardKeys::TargetActor));
		}
	}

	// Character actor locations already represent the capsule center, so avoid adding height that would push shots over the player.
	const FVector TargetLocation = IsValid(TargetActor)
		? TargetActor->GetActorLocation()
		: AvatarPawn->GetActorLocation() + AvatarPawn->GetActorForwardVector() * 1000.0f;
	FVector SpawnForwardDirection = (TargetLocation - AvatarPawn->GetActorLocation()).GetSafeNormal();
	if (SpawnForwardDirection.IsNearlyZero())
	{
		SpawnForwardDirection = AvatarPawn->GetActorForwardVector().GetSafeNormal();
	}

	const FVector SpawnLocation = AvatarPawn->GetActorLocation()
		+ SpawnForwardDirection * ProjectileSpawnForwardOffset
		+ FVector(0.0f, 0.0f, ProjectileSpawnHeightOffset);
	// Recalculate from the elevated muzzle position; reusing the pawn-origin direction offsets the whole trajectory upward.
	FVector AimDirection = (TargetLocation - SpawnLocation).GetSafeNormal();
	if (AimDirection.IsNearlyZero())
	{
		AimDirection = SpawnForwardDirection;
	}
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = AvatarPawn;
	SpawnParameters.Instigator = AvatarPawn;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AGP_EnemyRangedProjectile* Projectile = AvatarPawn->GetWorld()->SpawnActor<AGP_EnemyRangedProjectile>(
		ProjectileClass,
		SpawnLocation,
		AimDirection.Rotation(),
		SpawnParameters);
	if (IsValid(Projectile))
	{
		Projectile->LaunchToward(AimDirection);
	}
}
