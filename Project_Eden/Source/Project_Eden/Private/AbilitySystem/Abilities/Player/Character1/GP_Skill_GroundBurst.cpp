#include "AbilitySystem/Abilities/Player/Character1/GP_Skill_GroundBurst.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GP_SkillData.h"
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
		UGP_SkillData* SkillData = nullptr;
		if (const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle))
		{
			SkillData = Cast<UGP_SkillData>(Spec->SourceObject.Get());
		}

		FRotator AimRotation = Avatar->GetActorRotation();
		if (AController* Controller = Avatar->GetController())
		{
			AimRotation = Controller->GetControlRotation();
		}

		AimRotation.Pitch = 0.f;
		AimRotation.Roll = 0.f;

		const FVector TargetLocation = Avatar->GetActorLocation() + AimRotation.Vector() * TargetForwardOffset;
		const FVector TraceStart = TargetLocation + FVector::UpVector * TraceHeight;
		const FVector TraceEnd = TargetLocation - FVector::UpVector * TraceDepth;

		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GroundBurstTrace), false);
		QueryParams.AddIgnoredActor(Avatar);

		FHitResult HitResult;
		const bool bHitGround = Avatar->GetWorld()->LineTraceSingleByChannel(
			HitResult,
			TraceStart,
			TraceEnd,
			GroundTraceChannel,
			QueryParams
		);

		const FVector BurstLocation = bHitGround ? HitResult.Location : TargetLocation;
		const TArray<AActor*> HitActors = UGP_BlueprintLibrary::SphereOverlapActorsAtLocation(
			Avatar,
			BurstLocation,
			BurstRadius,
			Avatar,
			bDrawDebugs
		);

		if (DamageEffectClass)
		{
			UGP_BlueprintLibrary::ApplyGameplayEffectToActors(Avatar, HitActors, DamageEffectClass, GetAbilityLevel(), SkillData);
		}

		if (HitEventTag.IsValid())
		{
			UGP_BlueprintLibrary::SendGameplayEventToActors(Avatar, HitActors, HitEventTag);
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

UGP_Skill_GroundBurst::UGP_Skill_GroundBurst()
{
	HitEventTag = GPTags::Event::Enemy::HitReact;
}
