// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Player/GP_Primary.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/GP_WeaponAttributeSet.h"
#include "Characters/GP_BaseCharacter.h"
#include "Engine/OverlapResult.h"
#include "GameplayTags/GP_Tags.h"
#include "Player/GP_PlayerState.h"

namespace
{
int32 GetWeaponDamageAmount(const AActor* SourceActor)
{
	const APawn* SourcePawn = Cast<APawn>(SourceActor);
	if (!IsValid(SourcePawn))
	{
		return 1;
	}

	const AGP_PlayerState* PlayerState = SourcePawn->GetPlayerState<AGP_PlayerState>();
	if (!IsValid(PlayerState) || !IsValid(PlayerState->GetWeaponAttributeSet()))
	{
		return 1;
	}

	const UGP_WeaponAttributeSet* WeaponAttributeSet = PlayerState->GetWeaponAttributeSet();
	// 지금은 단순 표시용 합산식으로 계산하고, 이후 실제 전투 계산식으로 교체하기 쉽게 분리했다.
	const float DamageAmount = WeaponAttributeSet->GetAttackPower() + (WeaponAttributeSet->GetMagicPower() * 0.35f);
	return FMath::Max(1, FMath::RoundToInt(DamageAmount));
}

EWeaponElement GetWeaponElement(const AActor* SourceActor)
{
	const APawn* SourcePawn = Cast<APawn>(SourceActor);
	if (!IsValid(SourcePawn))
	{
		return EWeaponElement::Fire;
	}

	const AGP_PlayerState* PlayerState = SourcePawn->GetPlayerState<AGP_PlayerState>();
	if (!IsValid(PlayerState))
	{
		return EWeaponElement::Fire;
	}

	return PlayerState->GetEquippedWeaponElement();
}
}

TArray<AActor*> UGP_Primary::HitboxOverlapTest()
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
	const AActor* SourceActor = GetAvatarActorFromActorInfo();
	const int32 DamageAmount = GetWeaponDamageAmount(SourceActor);
	const EWeaponElement WeaponElement = GetWeaponElement(SourceActor);

	for (AActor* HitActor : ActorsHit)
	{
		if (AGP_BaseCharacter* HitCharacter = Cast<AGP_BaseCharacter>(HitActor))
		{
			// 피격 대상 머리 위에 원소 색상을 반영한 데미지 숫자를 띄운다.
			HitCharacter->ShowDamageNumber(DamageAmount, WeaponElement);
		}

		FGameplayEventData Payload;
		Payload.Instigator = GetAvatarActorFromActorInfo();
		Payload.EventMagnitude = static_cast<float>(DamageAmount);
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(HitActor, GPTags::Events::Enemy::HitReact, Payload);
	}
}

void UGP_Primary::DrawDebugsHitBoxOverlap(const TArray<FOverlapResult>& OverlapResults, const FVector& HitBoxLocation) const
{
	DrawDebugSphere(GetWorld(), HitBoxLocation, HitBoxRadius, 16, FColor::Red, false,  3.f);
	for (const FOverlapResult& Result : OverlapResults)
	{
		FVector DebugLocation = Result.GetActor()->GetActorLocation();
		DebugLocation.Z += 100.f;
		DrawDebugSphere(GetWorld(), DebugLocation, 30.f, 10, FColor::Green, false,  3.f);
	}
}
