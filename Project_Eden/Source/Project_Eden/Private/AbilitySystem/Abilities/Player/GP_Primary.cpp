// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Player/GP_Primary.h"

#include "Engine/OverlapResult.h"

void UGP_Primary::HitboxOverlapTest()
{
	TArray<AActor*> ActorToIgnore;
	ActorToIgnore.Add(GetAvatarActorFromActorInfo());
	
	FCollisionQueryParams CollisionQueryParams;
	CollisionQueryParams.AddIgnoredActors(ActorToIgnore);
	
	FCollisionResponseParams CollisionResponseParams;
	CollisionResponseParams.CollisionResponse.SetAllChannels(ECR_Ignore);
	CollisionResponseParams.CollisionResponse.SetResponse(ECC_Pawn,ECR_Block);
	
	TArray<FOverlapResult> OverlapResults;
	FCollisionShape CollisionShapeSphere = FCollisionShape::MakeSphere(HitBoxRadius); 
	
	const FVector Forward = GetAvatarActorFromActorInfo()->GetActorForwardVector() * HitBoxForwardOffset;
	const FVector HitBoxLocation = GetAvatarActorFromActorInfo()->GetActorLocation() + Forward + FVector(0.f,0.f,HitBoxElevationOffset);
	
	GetWorld()->OverlapMultiByChannel(OverlapResults, HitBoxLocation, FQuat::Identity, 
		ECC_Visibility, CollisionShapeSphere, CollisionQueryParams,CollisionResponseParams);

	if (bDrawDebugs)
	{
		DrawDebugSphere(GetWorld(), HitBoxLocation, HitBoxRadius, 16, FColor::Red, false,  3.f);
		for (const FOverlapResult& Result : OverlapResults)
		{
			FVector DebugLocation = Result.GetActor()->GetActorLocation();
			DebugLocation.Z += 100.f;
			DrawDebugSphere(GetWorld(), DebugLocation, 30.f, 10, FColor::Green, false,  3.f);
		}
	}
	
}
