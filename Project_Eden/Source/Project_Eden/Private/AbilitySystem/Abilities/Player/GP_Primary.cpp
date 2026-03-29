#include "AbilitySystem/Abilities/Player/GP_Primary.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/GP_AttributeSet.h"
#include "Engine/OverlapResult.h"
#include "GameplayTags/GP_Tags.h"


TArray<AActor*> UGP_Primary::HitboxOverlapTest()
{
	TArray<AActor*> ActorToIgnore;
	ActorToIgnore.Add(GetAvatarActorFromActorInfo());

	FCollisionQueryParams CollisionQueryParams;
	CollisionQueryParams.AddIgnoredActors(ActorToIgnore);

	FCollisionResponseParams CollisionResponseParams;
	CollisionResponseParams.CollisionResponse.SetAllChannels(ECR_Ignore);
	CollisionResponseParams.CollisionResponse.SetResponse(ECC_Pawn, ECR_Block);

	TArray<FOverlapResult> OverlapResults;
	FCollisionShape CollisionShapeSphere = FCollisionShape::MakeSphere(HitBoxRadius);

	const FVector Forward = GetAvatarActorFromActorInfo()->GetActorForwardVector() * HitBoxForwardOffset;
	const FVector HitBoxLocation = GetAvatarActorFromActorInfo()->GetActorLocation() + Forward + FVector(
		0.f, 0.f, HitBoxElevationOffset);

	GetWorld()->OverlapMultiByChannel(OverlapResults, HitBoxLocation, FQuat::Identity,
	                                  ECC_Visibility, CollisionShapeSphere, CollisionQueryParams,
	                                  CollisionResponseParams);

	TArray<AActor*> ActorsHit;
	for (const FOverlapResult& Result : OverlapResults)
	{
		if (!IsValid(Result.GetActor())) continue;
		ActorsHit.AddUnique(Result.GetActor());
	}

	if (bDrawDebugs)
	{
		DrawDebugsHitBoxOverlap(OverlapResults, HitBoxLocation);
	}

	return ActorsHit;
}

void UGP_Primary::SendHitReactEventToActors(const TArray<AActor*>& ActorsHit)
{
	for (AActor* HitActor : ActorsHit)
	{
		FGameplayEventData Payload;
		Payload.Instigator = GetAvatarActorFromActorInfo();
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(HitActor, GPTags::Events::Enemy::HitReact, Payload);
	}
}

void UGP_Primary::DrawDebugsHitBoxOverlap(const TArray<FOverlapResult>& OverlapResults,
                                          const FVector& HitBoxLocation) const
{
	DrawDebugSphere(GetWorld(), HitBoxLocation, HitBoxRadius, 16, FColor::Red, false, 3.f);
	for (const FOverlapResult& Result : OverlapResults)
	{
		FVector DebugLocation = Result.GetActor()->GetActorLocation();
		DebugLocation.Z += 100.f;
		DrawDebugSphere(GetWorld(), DebugLocation, 30.f, 10, FColor::Green, false, 3.f);
	}
}
