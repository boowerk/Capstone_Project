#include "Utils/GP_BlueprintLibrary.h"

#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

EHitDirection UGP_BlueprintLibrary::GetHitDirection(const FVector& TargetForward, const FVector& ToInstigator)
{
	// Dot Product(내적)으로 공격자가 앞(1.0), 뒤(-1.0), 또는 측면(0.0)에 있는지 판별
	const float ForwardDot = FVector::DotProduct(TargetForward, ToInstigator);
	
	const float DiagonalThreshold = 0.5f; // 45도 각도 경계값을 위한 임계값
	
	if (ForwardDot < -DiagonalThreshold)
	{
		return EHitDirection::Back;
	}

	if (ForwardDot < DiagonalThreshold)
	{
		// 외적의 Z축을 통해 왼쪽인지 오른쪽인지 판별
		const FVector CrossProduct = FVector::CrossProduct(TargetForward, ToInstigator);
		if (CrossProduct.Z < 0.0f)
		{
			return EHitDirection::Left;
		}
		
		return EHitDirection::Right;
	}
	
	// 공격자가 타겟 앞에 있는 경우
	return EHitDirection::Forward;
}

FName UGP_BlueprintLibrary::GetHitDirectionName(const EHitDirection& HitDirection)
{
	switch (HitDirection)
	{
	case EHitDirection::Left:		return FName("Left");
	case EHitDirection::Right:		return FName("Right");
	case EHitDirection::Forward:	return FName("Forward");
	case EHitDirection::Back:		return FName("Back");
	default:						return FName("None");
	}
}



TArray<AActor*> UGP_BlueprintLibrary::SphereMeleeHitBoxOverlap(AActor* AvatarActor, float Radius, 
	float ForwardOffset, float ElevationOffset, bool bDrawDebug)
{
	// ActorsHit를 미리 생성하여 AvatarActor 유효성검사시 의미없는 연산을 막게 변경함 - 슝민 
	TArray<AActor*> ActorsHit;
	if (!IsValid(AvatarActor)) return ActorsHit;

	UWorld* World = AvatarActor->GetWorld();
	if (!IsValid(World)) return ActorsHit;

	TArray<AActor*> ActorToIgnore;
	ActorToIgnore.Add(AvatarActor);

	FCollisionQueryParams CollisionQueryParams;
	CollisionQueryParams.AddIgnoredActors(ActorToIgnore);

	FCollisionResponseParams CollisionResponseParams;
	CollisionResponseParams.CollisionResponse.SetAllChannels(ECR_Ignore);
	CollisionResponseParams.CollisionResponse.SetResponse(ECC_Pawn, ECR_Block);

	// 위치 계산
	
	TArray<FOverlapResult> OverlapResults;
	FCollisionShape CollisionShapeSphere = FCollisionShape::MakeSphere(Radius);
	
	const FVector Forward = AvatarActor->GetActorForwardVector() * ForwardOffset;
	const FVector HitBoxLocation = AvatarActor->GetActorLocation() + Forward + FVector(0.f, 0.f, ElevationOffset);

	
	World->OverlapMultiByChannel(OverlapResults, HitBoxLocation, FQuat::Identity,
		ECC_Visibility, CollisionShapeSphere, CollisionQueryParams, CollisionResponseParams);

	for (const FOverlapResult& Result : OverlapResults)
	{
		if (AActor* HitActor = Result.GetActor())
		{
			ActorsHit.AddUnique(HitActor);
		}
	}
	
	if (bDrawDebug) // UGP_Primary::DrawDebugsHitBoxOverlap에서 기능 이전함
	{
		DrawDebugSphere(World, HitBoxLocation, Radius, 16, FColor::Red, false, 3.f);
		for (const FOverlapResult& Result : OverlapResults)
		{
			if (Result.GetActor())
			{
				FVector DebugLocation = Result.GetActor()->GetActorLocation();
				DebugLocation.Z += 100.f;
				DrawDebugSphere(World, DebugLocation, 30.f, 10, FColor::Green, false, 3.f);
			}
		}
	}

	return ActorsHit;
}

void UGP_BlueprintLibrary::SendGameplayEventToActors(AActor* Instigator, const TArray<AActor*>& TargetActors, FGameplayTag EventTag)
{
	if (!IsValid(Instigator)) return;

	for (AActor* HitActor : TargetActors)
	{
		FGameplayEventData Payload;
		Payload.Instigator = Instigator;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(HitActor, EventTag, Payload);
	}
}

void UGP_BlueprintLibrary::ApplyGameplayEffectToActors(AActor* Instigator, const TArray<AActor*>& TargetActors, TSubclassOf<UGameplayEffect> EffectClass, float EffectLevel)
{
	if (!IsValid(Instigator) || !IsValid(EffectClass)) return;

	UAbilitySystemComponent* InstigatorASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Instigator);
	if (!IsValid(InstigatorASC)) return;

	for (AActor* TargetActor : TargetActors)
	{
		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
		if (IsValid(TargetASC))
		{
			FGameplayEffectContextHandle ContextHandle = InstigatorASC->MakeEffectContext();
			ContextHandle.AddInstigator(Instigator, Instigator);
			
			FGameplayEffectSpecHandle SpecHandle = InstigatorASC->MakeOutgoingSpec(EffectClass, EffectLevel, ContextHandle);
			InstigatorASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		}
	}
}
