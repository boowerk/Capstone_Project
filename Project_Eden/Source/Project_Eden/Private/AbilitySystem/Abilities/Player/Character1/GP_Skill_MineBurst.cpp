#include "AbilitySystem/Abilities/Player/Character1/GP_Skill_MineBurst.h"

#include "AbilitySystemComponent.h"
#include "Actors/GP_MineBurstActor.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"

void UGP_Skill_MineBurst::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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

	if (!Avatar || !ASC || !MineActorClass || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (HasAuthority(&ActivationInfo))
	{
		UGP_SkillData* SkillData = GetSkillDataFromSpec(Handle, ActorInfo);

		FRotator AimRotation = Avatar->GetActorRotation();
		if (AController* Controller = Avatar->GetController())
		{
			AimRotation = Controller->GetControlRotation();
		}

		AimRotation.Pitch = 0.f;
		AimRotation.Roll = 0.f;

		const FVector TargetLocation = Avatar->GetActorLocation() + AimRotation.Vector() * PlaceForwardOffset;
		const FVector TraceStart = TargetLocation + FVector::UpVector * TraceHeight;
		const FVector TraceEnd = TargetLocation - FVector::UpVector * TraceDepth;

		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MineBurstTrace), false);
		QueryParams.AddIgnoredActor(Avatar);

		FHitResult HitResult;
		const bool bHitGround = Avatar->GetWorld()->LineTraceSingleByChannel(
			HitResult,
			TraceStart,
			TraceEnd,
			GroundTraceChannel,
			QueryParams
		);

		const FVector MineLocation = bHitGround ? HitResult.Location : TargetLocation;

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Avatar;
		SpawnParams.Instigator = Avatar;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AGP_MineBurstActor* MineActor = Avatar->GetWorld()->SpawnActor<AGP_MineBurstActor>(
			MineActorClass,
			MineLocation,
			FRotator::ZeroRotator,
			SpawnParams
		);

		if (MineActor)
		{
			MineActor->SetSkillData(SkillData);
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
