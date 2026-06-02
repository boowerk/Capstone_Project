#include "Animation/GP_AnimNotify_SendGameplayEvent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameplayTags/GP_Tags.h"

void UGP_AnimNotify_SendGameplayEvent::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!IsValid(MeshComp) || !GameplayEventTag.IsValid())
	{
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!IsValid(OwnerActor))
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.Instigator = OwnerActor;

	// Primary attack montages were migrated with both the old ability tag and the newer hit-frame event tag.
	const auto SendEvent = [&Payload, OwnerActor](FGameplayTag EventTag)
	{
		if (EventTag.IsValid())
		{
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerActor, EventTag, Payload);
		}
	};

	// 몽타주 노티파이에서 바로 Gameplay Event를 보내 어빌리티가 정확한 타격 프레임을 받을 수 있게 한다.
	SendEvent(GameplayEventTag);
	if (GameplayEventTag.MatchesTagExact(GPTags::Ability::Skill::Primary))
	{
		SendEvent(GPTags::Event::Player::AttackHit);
	}
	else if (GameplayEventTag.MatchesTagExact(GPTags::Event::Player::AttackHit))
	{
		SendEvent(GPTags::Ability::Skill::Primary);
	}
	// if (GEngine)
	// {
	// 	FString DebugMsg = FString::Printf(TEXT("노티파이 발동 액터: %s / 태그: %s"), *OwnerActor->GetName(), *GameplayEventTag.ToString());
	// 	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, DebugMsg);
	// }
}

FString UGP_AnimNotify_SendGameplayEvent::GetNotifyName_Implementation() const
{
	return GameplayEventTag.IsValid()
		? FString::Printf(TEXT("SendEvent:%s"), *GameplayEventTag.ToString())
		: TEXT("SendGameplayEvent");
}
