#include "AbilitySystem/Abilities/Player/Character1/GP_Skill_IceMist.h"

#include "AbilitySystem/Abilities/GP_SkillData.h"
#include "Actors/GP_IceMistArea.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameplayTags/GP_Tags.h"

UGP_Skill_IceMist::UGP_Skill_IceMist()
{
	SelectionMode = EGP_SkillSelectionMode::Projectile;
	bAllowSecondaryConfirm = false;
	bRequireBlockingHit = false;
	HitEventTag = GPTags::Event::Enemy::HitReact;
	IceMistAreaClass = AGP_IceMistArea::StaticClass();
}

void UGP_Skill_IceMist::ExecuteConfirmedSkill_Implementation(
	const FGP_SkillTargetData& TargetData)
{
	if (!HasAuthority(&CurrentActivationInfo))
	{
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UWorld* World = GetWorld();
	if (!IsValid(AvatarActor) || !World || !IceMistAreaClass)
	{
		return;
	}

	UGP_SkillData* SkillData = GetSkillDataFromSpec(CurrentSpecHandle, CurrentActorInfo);
	const float RadiusMultiplier = GetSkillAugmentRadiusMultiplier(SkillData, CurrentActorInfo);
	const float FinalRadius = MistRadius * RadiusMultiplier;

	FRotator LaunchRotation = TargetData.AimDirection.Rotation();
	LaunchRotation.Pitch = FMath::ClampAngle(LaunchRotation.Pitch, -5.0f, 10.0f);
	LaunchRotation.Roll = 0.0f;
	const FVector LaunchDirection = LaunchRotation.Vector();
	const FVector SpawnLocation =
		AvatarActor->GetActorLocation()
		+ LaunchDirection * SpawnForwardOffset
		+ FVector::UpVector * SpawnUpOffset;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = AvatarActor;
	SpawnParams.Instigator = Cast<APawn>(AvatarActor);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.bDeferConstruction = true;

	AGP_IceMistArea* IceMistArea = World->SpawnActor<AGP_IceMistArea>(
		IceMistAreaClass,
		SpawnLocation,
		LaunchRotation,
		SpawnParams);
	if (!IsValid(IceMistArea))
	{
		return;
	}

	IceMistArea->InitializeIceMist(
		AvatarActor,
		SkillData,
		DamageEffectClass,
		SlowEffectClass,
		HitEventTag,
		GetAbilityLevel(),
		FinalRadius,
		MistDuration,
		DamageInterval,
		LaunchDirection,
		LaunchSpeed,
		MoveDuration,
		ShouldDrawDebug());
	IceMistArea->FinishSpawning(
		FTransform(LaunchRotation, SpawnLocation));
}
