#include "AbilitySystem/Abilities/Enemy/GP_HitReact.h"

void UGP_HitReact::CacheHitDirectionVectors(AActor* Instigator)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	
	if (!IsValid(AvatarActor) || !IsValid(Instigator)) return;
	
	AvatarForward = GetAvatarActorFromActorInfo()->GetActorForwardVector();

	const FVector AvatarLocation = GetAvatarActorFromActorInfo()->GetActorLocation();
	const FVector InstigatorLocation = Instigator->GetActorLocation();

	ToInstigator = InstigatorLocation - AvatarLocation;
	ToInstigator.Normalize();
}
