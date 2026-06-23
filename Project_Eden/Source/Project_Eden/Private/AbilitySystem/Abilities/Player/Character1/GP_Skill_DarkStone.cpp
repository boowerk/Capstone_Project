#include "AbilitySystem/Abilities/Player/Character1/GP_Skill_DarkStone.h"

#include "AbilitySystem/Abilities/GP_SkillData.h"
#include "Actors/GP_GroundLaunchProjectile.h"
#include "GameFramework/Character.h"
#include "GameplayTags/GP_Tags.h"

UGP_Skill_DarkStone::UGP_Skill_DarkStone()
{
	SelectionMode = EGP_SkillSelectionMode::Projectile;
	bEndAbilityAfterConfirmedExecute = true;
	GroundTraceHeight = 500.f;
	GroundTraceDepth = 1000.f;
}

void UGP_Skill_DarkStone::ExecuteConfirmedSkill_Implementation(const FGP_SkillTargetData& TargetData)
{
	if (!HasAuthority(&CurrentActivationInfo))
	{
		return;
	}

	ACharacter* Avatar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(Avatar))
	{
		return;
	}

	UGP_SkillData* SkillData = GetSkillDataFromSpec(CurrentSpecHandle, CurrentActorInfo);
	const TSubclassOf<AActor> SpawnActorClass = GetSkillSpawnActorClass(SkillData, ProjectileClass);
	if (!SpawnActorClass)
	{
		return;
	}

	FVector LaunchDirection = TargetData.AimDirection.GetSafeNormal();
	if (LaunchDirection.IsNearlyZero())
	{
		LaunchDirection = Avatar->GetActorForwardVector();
	}

	FRotator LaunchRotation = LaunchDirection.Rotation();
	LaunchRotation.Pitch = FMath::ClampAngle(LaunchRotation.Pitch, -5.f, 10.f);
	LaunchRotation.Roll = 0.f;
	LaunchDirection = LaunchRotation.Vector();

	const float RangeMultiplier = GetSkillAugmentRangeMultiplier(SkillData, CurrentActorInfo);
	FVector GroundDirection = LaunchDirection.GetSafeNormal2D();
	if (GroundDirection.IsNearlyZero())
	{
		GroundDirection = Avatar->GetActorForwardVector().GetSafeNormal2D();
	}

	const FVector TargetGroundLocation =
		Avatar->GetActorLocation() + GroundDirection * (GroundForwardOffset * RangeMultiplier);
	const FVector TraceStart = TargetGroundLocation + FVector::UpVector * GroundTraceHeight;
	const FVector TraceEnd = TargetGroundLocation - FVector::UpVector * GroundTraceDepth;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(DarkStoneGroundTrace), false);
	QueryParams.AddIgnoredActor(Avatar);

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);

	FHitResult GroundHit;
	const bool bHitGround = Avatar->GetWorld()->LineTraceSingleByObjectType(
		GroundHit,
		TraceStart,
		TraceEnd,
		ObjectQueryParams,
		QueryParams);
	const FVector GroundLocation = bHitGround ? GroundHit.Location : TargetGroundLocation;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Avatar;
	SpawnParams.Instigator = Avatar;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* SpawnedActor = Avatar->GetWorld()->SpawnActor<AActor>(
		SpawnActorClass,
		GroundLocation,
		LaunchRotation,
		SpawnParams);
	AGP_GroundLaunchProjectile* Projectile = Cast<AGP_GroundLaunchProjectile>(SpawnedActor);
	if (!Projectile)
	{
		if (SpawnedActor)
		{
			SpawnedActor->Destroy();
		}
		return;
	}

	const FGameplayTag TechElementTag = GetCurrentTechElementTag(CurrentActorInfo);
	const float RadiusMultiplier = GetSkillAugmentRadiusMultiplier(SkillData, CurrentActorInfo);

	Projectile->SetSkillData(SkillData);
	Projectile->SetProjectileVisualSystem(GetProjectileVisualSystem(SkillData, TechElementTag));
	Projectile->SetImpactVisualActorClass(GetSkillVisualActorClass(
		SkillData,
		nullptr,
		TechElementTag,
		GPTags::Ability::Skill::Visual::Impact));
	Projectile->ApplyExplosionRadiusMultiplier(RadiusMultiplier);
	Projectile->InitializeGroundLaunch(
		GroundLocation,
		LaunchDirection,
		RiseDepth,
		RiseHeight,
		RiseDuration,
		LaunchDelay,
		LaunchSpeed);
}
