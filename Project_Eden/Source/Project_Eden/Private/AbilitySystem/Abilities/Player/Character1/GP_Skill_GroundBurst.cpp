#include "AbilitySystem/Abilities/Player/Character1/GP_Skill_GroundBurst.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameplayTags/GP_Tags.h"
#include "Utils/GP_BlueprintLibrary.h"

void UGP_Skill_GroundBurst::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                            const FGameplayAbilityActorInfo* ActorInfo,
                                            const FGameplayAbilityActivationInfo ActivationInfo,
                                            const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!ActorInfo)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ACharacter* Avatar = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

	if (!Avatar || !ASC || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (HasAuthority(&ActivationInfo))
	{
		UGP_SkillData* SkillData = GetSkillDataFromSpec(Handle, ActorInfo);
		const FGameplayTag TechElementTag = GetCurrentTechElementTag(ActorInfo);
		const float RadiusMultiplier = GetSkillAugmentRadiusMultiplier(SkillData, ActorInfo);
		const float RangeMultiplier = GetSkillAugmentRangeMultiplier(SkillData, ActorInfo);
		const float FinalBurstRadius = BurstRadius * RadiusMultiplier;

		FRotator AimRotation = Avatar->GetActorRotation();
		if (AController* Controller = Avatar->GetController())
		{
			AimRotation = Controller->GetControlRotation();
		}

		AimRotation.Pitch = 0.f;
		AimRotation.Roll = 0.f;

		const FVector TargetLocation = Avatar->GetActorLocation() + AimRotation.Vector() * (TargetForwardOffset * RangeMultiplier);
		const FVector TraceStart = TargetLocation + FVector::UpVector * TraceHeight;
		const FVector TraceEnd = TargetLocation - FVector::UpVector * TraceDepth;

		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GroundBurstTrace), false);
		QueryParams.AddIgnoredActor(Avatar);
		FCollisionObjectQueryParams ObjectQueryParams;
		ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);

		FHitResult HitResult;
		const bool bHitGround = Avatar->GetWorld()->LineTraceSingleByObjectType(
			HitResult,
			TraceStart,
			TraceEnd,
			ObjectQueryParams,
			QueryParams
		);

		const FVector BurstLocation = bHitGround ? HitResult.Location : TargetLocation;
		SpawnVisualActor(Avatar, GetSkillVisualActorClass(SkillData, BurstVisualActorClass, TechElementTag), BurstLocation, FRotator::ZeroRotator, RadiusMultiplier);

		const TArray<AActor*> HitActors = UGP_BlueprintLibrary::SphereOverlapActorsAtLocation(
			Avatar,
			BurstLocation,
			FinalBurstRadius,
			Avatar,
			bDrawDebugs
		);

		UGP_BlueprintLibrary::ApplyGameplayEffectAndEventToActors(Avatar, HitActors, DamageEffectClass, HitEventTag, GetAbilityLevel(), SkillData);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

UGP_Skill_GroundBurst::UGP_Skill_GroundBurst()
{
	HitEventTag = GPTags::Event::Enemy::HitReact;
}
