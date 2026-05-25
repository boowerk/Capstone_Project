#include "AbilitySystem/Abilities/Player/Character1/GP_Skill_ConeSlash.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameplayTags/GP_Tags.h"
#include "Utils/GP_BlueprintLibrary.h"

void UGP_Skill_ConeSlash::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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

		FRotator AimRotation = Avatar->GetActorRotation();
		if (AController* Controller = Avatar->GetController())
		{
			AimRotation = Controller->GetControlRotation();
		}

		AimRotation.Pitch = 0.f;
		AimRotation.Roll = 0.f;

		const FVector Origin = Avatar->GetActorLocation();
		const FVector Forward = AimRotation.Vector().GetSafeNormal2D();
		const FVector VisualLocation =
			Origin
			+ Forward * VisualForwardOffset
			+ FVector::UpVector * VisualHeightOffset;

		SpawnVisualActor(Avatar, GetSkillVisualActorClass(SkillData, SlashVisualActorClass, TechElementTag), VisualLocation, AimRotation);

		const TArray<AActor*> CandidateActors = UGP_BlueprintLibrary::SphereOverlapActorsAtLocation(
			Avatar,
			Origin,
			Range,
			Avatar,
			bDrawDebugs
		);

		TArray<AActor*> HitActors;
		const float MinForwardDot = FMath::Cos(FMath::DegreesToRadians(ConeAngle * 0.5f));

		for (AActor* CandidateActor : CandidateActors)
		{
			if (!IsValid(CandidateActor))
			{
				continue;
			}

			const FVector ToTarget = (CandidateActor->GetActorLocation() - Origin).GetSafeNormal2D();
			if (FVector::DotProduct(Forward, ToTarget) >= MinForwardDot)
			{
				HitActors.Add(CandidateActor);
			}
		}

		UGP_BlueprintLibrary::ApplyGameplayEffectAndEventToActors(Avatar, HitActors, DamageEffectClass, HitEventTag, GetAbilityLevel(), SkillData);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

UGP_Skill_ConeSlash::UGP_Skill_ConeSlash()
{
	HitEventTag = GPTags::Event::Enemy::HitReact;
}
