#include "Utils/GP_BlueprintLibrary.h"

#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GP_GameplayAbility.h"
#include "AbilitySystem/Abilities/GP_SkillData.h"
#include "Actors/GP_BullChargeActor.h"
#include "Characters/GP_EnemyCharacter.h"
#include "Characters/GP_PlayerCharacter.h"
#include "GameplayEffect.h"
#include "GameplayTags/GP_Tags.h"
#include "GameFramework/Pawn.h"
#include "Player/GP_PlayerState.h"

namespace
{
const AGP_PlayerState* GetGPPlayerStateFromActor(const AActor* Actor)
{
	if (const APawn* Pawn = Cast<APawn>(Actor))
	{
		if (const AGP_PlayerState* PlayerState = Pawn->GetPlayerState<AGP_PlayerState>())
		{
			return PlayerState;
		}
	}

	if (const APawn* InstigatorPawn = Actor ? Actor->GetInstigator() : nullptr)
	{
		if (const AGP_PlayerState* PlayerState = InstigatorPawn->GetPlayerState<AGP_PlayerState>())
		{
			return PlayerState;
		}
	}

	return nullptr;
}

FGameplayTag GetCurrentTechElementTagFromActor(const AActor* Actor)
{
	if (const AGP_PlayerState* PlayerState = GetGPPlayerStateFromActor(Actor))
	{
		return PlayerState->GetCurrentTechElementTag();
	}

	return FGameplayTag();
}

float GetSkillAugmentDamageMultiplierFromActor(const AActor* Actor, const UGP_SkillData* SkillData)
{
	if (!SkillData || !SkillData->SkillIdTag.IsValid())
	{
		return 1.0f;
	}

	if (const AGP_PlayerState* PlayerState = GetGPPlayerStateFromActor(Actor))
	{
		return PlayerState->GetSkillAugmentDamageMultiplier(SkillData->SkillIdTag);
	}

	return 1.0f;
}

bool IsPlayerCombatActor(const AActor* Actor)
{
	return IsValid(Actor)
		&& (Actor->IsA<AGP_PlayerCharacter>() || GetGPPlayerStateFromActor(Actor) != nullptr);
}

FGameplayTag ConvertTechElementToDamageElement(FGameplayTag TechElementTag)
{
	if (TechElementTag.MatchesTagExact(GPTags::Tech::Element::Pyros)) { return GPTags::Damage::Element::Pyros; }
	if (TechElementTag.MatchesTagExact(GPTags::Tech::Element::Hydro)) { return GPTags::Damage::Element::Hydro; }
	if (TechElementTag.MatchesTagExact(GPTags::Tech::Element::Volt)) { return GPTags::Damage::Element::Volt; }
	if (TechElementTag.MatchesTagExact(GPTags::Tech::Element::Aero)) { return GPTags::Damage::Element::Aero; }
	if (TechElementTag.MatchesTagExact(GPTags::Tech::Element::Lux)) { return GPTags::Damage::Element::Lux; }
	if (TechElementTag.MatchesTagExact(GPTags::Tech::Element::Chaos)) { return GPTags::Damage::Element::Chaos; }
	if (TechElementTag.MatchesTagExact(GPTags::Tech::Element::Brute)) { return GPTags::Damage::Element::Brute; }

	return FGameplayTag();
}

bool IsMatadorBullCounterTarget(AActor* Instigator, AActor* TargetActor)
{
	return IsValid(Instigator)
		&& IsValid(TargetActor)
		&& TargetActor->IsA<AGP_BullChargeActor>()
		&& !Instigator->IsA<AGP_EnemyCharacter>();
}

bool TryHandleMatadorBullCounter(AActor* Instigator, AActor* TargetActor)
{
	AGP_BullChargeActor* BullActor = Cast<AGP_BullChargeActor>(TargetActor);
	if (!IsMatadorBullCounterTarget(Instigator, BullActor))
	{
		return false;
	}

	const bool bRedirected = BullActor->TryRedirectTowardDecoy(Instigator);
	UE_LOG(LogTemp, Log, TEXT("[MatadorBull] Player counter %s. Instigator=%s Bull=%s State=%d"),
		bRedirected ? TEXT("succeeded") : TEXT("rejected"),
		*GetNameSafe(Instigator),
		*GetNameSafe(BullActor),
		static_cast<int32>(BullActor->GetChargeState()));
	return true;
}

}

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

	const FVector Forward = AvatarActor->GetActorForwardVector() * ForwardOffset;
	const FVector HitBoxLocation = AvatarActor->GetActorLocation() + Forward + FVector(0.f, 0.f, ElevationOffset);

	return SphereOverlapActorsAtLocation(AvatarActor, HitBoxLocation, Radius, AvatarActor, bDrawDebug);
}

TArray<AActor*> UGP_BlueprintLibrary::SphereOverlapActorsAtLocation(UObject* WorldContextObject, const FVector& Location, float Radius, AActor* ActorToIgnore, bool bDrawDebug)
{
	TArray<AActor*> ActorsHit;
	if (!IsValid(WorldContextObject)) return ActorsHit;

	UWorld* World = WorldContextObject->GetWorld();
	if (!IsValid(World)) return ActorsHit;

	TArray<AActor*> ActorsToIgnore;
	if (IsValid(ActorToIgnore))
	{
		ActorsToIgnore.Add(ActorToIgnore);
	}

	FCollisionQueryParams CollisionQueryParams;
	CollisionQueryParams.AddIgnoredActors(ActorsToIgnore);

	FCollisionResponseParams CollisionResponseParams;
	CollisionResponseParams.CollisionResponse.SetAllChannels(ECR_Ignore);
	CollisionResponseParams.CollisionResponse.SetResponse(ECC_Pawn, ECR_Block);
	CollisionResponseParams.CollisionResponse.SetResponse(ECC_WorldDynamic, ECR_Block);

	TArray<FOverlapResult> OverlapResults;
	FCollisionShape CollisionShapeSphere = FCollisionShape::MakeSphere(Radius);

	World->OverlapMultiByChannel(OverlapResults, Location, FQuat::Identity,
		ECC_Visibility, CollisionShapeSphere, CollisionQueryParams, CollisionResponseParams);

	for (const FOverlapResult& Result : OverlapResults)
	{
		if (AActor* HitActor = Result.GetActor())
		{
			// Keep friendly enemies out of the hit list before any ability-specific follow-up logic runs.
			if (CanApplyCombatEffect(ActorToIgnore, HitActor))
			{
				ActorsHit.AddUnique(HitActor);
			}
		}
	}
	
	if (bDrawDebug && UGP_GameplayAbility::IsSkillDebugDrawEnabled()) // UGP_Primary::DrawDebugsHitBoxOverlap에서 기능 이전함
	{
		DrawDebugSphere(World, Location, Radius, 16, FColor::Red, false, 3.f);
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

TArray<AActor*> UGP_BlueprintLibrary::BoxOverlapActorsAtLocation(UObject* WorldContextObject, const FVector& Location, const FVector& BoxExtent, const FRotator& Rotation, AActor* ActorToIgnore, bool bDrawDebug)
{
	TArray<AActor*> ActorsHit;
	if (!IsValid(WorldContextObject)) return ActorsHit;

	UWorld* World = WorldContextObject->GetWorld();
	if (!IsValid(World)) return ActorsHit;

	TArray<AActor*> ActorsToIgnore;
	if (IsValid(ActorToIgnore))
	{
		ActorsToIgnore.Add(ActorToIgnore);
	}

	FCollisionQueryParams CollisionQueryParams;
	CollisionQueryParams.AddIgnoredActors(ActorsToIgnore);

	FCollisionResponseParams CollisionResponseParams;
	CollisionResponseParams.CollisionResponse.SetAllChannels(ECR_Ignore);
	CollisionResponseParams.CollisionResponse.SetResponse(ECC_Pawn, ECR_Block);
	CollisionResponseParams.CollisionResponse.SetResponse(ECC_WorldDynamic, ECR_Block);

	TArray<FOverlapResult> OverlapResults;
	const FCollisionShape CollisionShapeBox = FCollisionShape::MakeBox(BoxExtent);
	const FQuat RotationQuat = Rotation.Quaternion();

	World->OverlapMultiByChannel(OverlapResults, Location, RotationQuat,
		ECC_Visibility, CollisionShapeBox, CollisionQueryParams, CollisionResponseParams);

	for (const FOverlapResult& Result : OverlapResults)
	{
		if (AActor* HitActor = Result.GetActor())
		{
			ActorsHit.AddUnique(HitActor);
		}
	}

	if (bDrawDebug && UGP_GameplayAbility::IsSkillDebugDrawEnabled())
	{
		DrawDebugBox(World, Location, BoxExtent, RotationQuat, FColor::Red, false, 3.f);
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

TArray<AActor*> UGP_BlueprintLibrary::ForwardArcMeleeHitBoxOverlap(AActor* AvatarActor, float Radius,
	float ForwardOffset, float ArcAngleDegrees, float ElevationOffset, bool bDrawDebug)
{
	TArray<AActor*> ActorsHit;
	if (!IsValid(AvatarActor)) return ActorsHit;

	UWorld* World = AvatarActor->GetWorld();
	if (!IsValid(World)) return ActorsHit;

	FCollisionQueryParams CollisionQueryParams;
	CollisionQueryParams.AddIgnoredActor(AvatarActor);

	FCollisionResponseParams CollisionResponseParams;
	CollisionResponseParams.CollisionResponse.SetAllChannels(ECR_Ignore);
	CollisionResponseParams.CollisionResponse.SetResponse(ECC_Pawn, ECR_Block);
	CollisionResponseParams.CollisionResponse.SetResponse(ECC_WorldDynamic, ECR_Block);

	TArray<FOverlapResult> OverlapResults;
	const FVector ForwardVector = AvatarActor->GetActorForwardVector().GetSafeNormal2D();
	const FVector HitBoxLocation = AvatarActor->GetActorLocation() + (ForwardVector * ForwardOffset) + FVector(0.f, 0.f, ElevationOffset);
	const FCollisionShape CollisionShapeSphere = FCollisionShape::MakeSphere(Radius);

	World->OverlapMultiByChannel(OverlapResults, HitBoxLocation, FQuat::Identity,
		ECC_Visibility, CollisionShapeSphere, CollisionQueryParams, CollisionResponseParams);

	const float HalfAngleDegrees = FMath::Clamp(ArcAngleDegrees * 0.5f, 0.0f, 180.0f);
	const float CosThreshold = FMath::Cos(FMath::DegreesToRadians(HalfAngleDegrees));
	const bool bFullCircle = HalfAngleDegrees >= 179.9f;

	for (const FOverlapResult& Result : OverlapResults)
	{
		AActor* HitActor = Result.GetActor();
		if (!IsValid(HitActor))
		{
			continue;
		}

		FVector DirectionToTarget = HitActor->GetActorLocation() - AvatarActor->GetActorLocation();
		DirectionToTarget.Z = 0.0f;
		if (DirectionToTarget.IsNearlyZero())
		{
			if (CanApplyCombatEffect(AvatarActor, HitActor))
			{
				ActorsHit.AddUnique(HitActor);
			}
			continue;
		}

		// The broad sphere keeps the overlap cheap; this dot check carves it into a forward sweep arc.
		if ((bFullCircle || FVector::DotProduct(ForwardVector, DirectionToTarget.GetSafeNormal()) >= CosThreshold)
			&& CanApplyCombatEffect(AvatarActor, HitActor))
		{
			ActorsHit.AddUnique(HitActor);
		}
	}

	if (bDrawDebug && UGP_GameplayAbility::IsSkillDebugDrawEnabled())
	{
		DrawDebugSphere(World, HitBoxLocation, Radius, 16, FColor::Orange, false, 3.f);
		const FVector Origin = AvatarActor->GetActorLocation() + FVector(0.f, 0.f, ElevationOffset);
		const FRotator LeftRotator(0.0f, -HalfAngleDegrees, 0.0f);
		const FRotator RightRotator(0.0f, HalfAngleDegrees, 0.0f);
		DrawDebugLine(World, Origin, Origin + LeftRotator.RotateVector(ForwardVector) * Radius, FColor::Yellow, false, 3.f, 0, 3.f);
		DrawDebugLine(World, Origin, Origin + RightRotator.RotateVector(ForwardVector) * Radius, FColor::Yellow, false, 3.f, 0, 3.f);

		for (AActor* HitActor : ActorsHit)
		{
			FVector DebugLocation = HitActor->GetActorLocation();
			DebugLocation.Z += 100.f;
			DrawDebugSphere(World, DebugLocation, 30.f, 10, FColor::Green, false, 3.f);
		}
	}

	return ActorsHit;
}

void UGP_BlueprintLibrary::SendGameplayEventToActors(AActor* Instigator, const TArray<AActor*>& TargetActors, FGameplayTag EventTag)
{
	if (!IsValid(Instigator)) return;

	for (AActor* HitActor : TargetActors)
	{
		if (TryHandleMatadorBullCounter(Instigator, HitActor))
		{
			continue;
		}

		if (!CanApplyCombatEffect(Instigator, HitActor))
		{
			continue;
		}

		FGameplayEventData Payload;
		Payload.Instigator = Instigator;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(HitActor, EventTag, Payload);
	}
}

bool UGP_BlueprintLibrary::CanApplyCombatEffect(AActor* Instigator, AActor* TargetActor)
{
	if (!IsValid(Instigator) || !IsValid(TargetActor) || Instigator == TargetActor)
	{
		return false;
	}

	if (IsMatadorBullCounterTarget(Instigator, TargetActor))
	{
		return true;
	}

	// All connected players are members of the same co-op party. Block the hit before
	// gameplay events, knockback, or projectile impact handling can affect a teammate.
	if (IsPlayerCombatActor(Instigator) && IsPlayerCombatActor(TargetActor))
	{
		return false;
	}

	// Enemy-vs-enemy friendly fire is disabled so bosses, summons, and regular enemies cannot damage each other.
	return !(Instigator->IsA<AGP_EnemyCharacter>() && TargetActor->IsA<AGP_EnemyCharacter>());
}

void UGP_BlueprintLibrary::ApplyGameplayEffectToActors(AActor* Instigator, const TArray<AActor*>& TargetActors, TSubclassOf<UGameplayEffect> EffectClass, float EffectLevel, UGP_SkillData* SkillData, float DamageScale)
{
	if (!IsValid(Instigator)) return;

	for (AActor* TargetActor : TargetActors)
	{
		if (TryHandleMatadorBullCounter(Instigator, TargetActor))
		{
			continue;
		}

		if (!CanApplyCombatEffect(Instigator, TargetActor))
		{
			continue;
		}

		if (!IsValid(EffectClass))
		{
			continue;
		}

		UAbilitySystemComponent* InstigatorASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Instigator);
		if (!IsValid(InstigatorASC))
		{
			continue;
		}

		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
		if (IsValid(TargetASC))
		{
			FGameplayEffectContextHandle ContextHandle = InstigatorASC->MakeEffectContext();
			ContextHandle.AddInstigator(Instigator, Instigator);
			
			FGameplayEffectSpecHandle SpecHandle = InstigatorASC->MakeOutgoingSpec(EffectClass, EffectLevel, ContextHandle);
			if (SpecHandle.IsValid())
			{
				const FGameplayTag DamageElementTag = ConvertTechElementToDamageElement(GetCurrentTechElementTagFromActor(Instigator));
				if (DamageElementTag.IsValid())
				{
					SpecHandle.Data->AddDynamicAssetTag(DamageElementTag);
				}

				if (SkillData)
				{
					const float SkillDamageMultiplier = GetSkillAugmentDamageMultiplierFromActor(Instigator, SkillData) * FMath::Max(0.0f, DamageScale);
					SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Data::Base, SkillData->BaseDamage);
					SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Data::BaseSpell, SkillData->BaseSpellDamage);
					SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Data::ToughnessBase, SkillData->ToughnessDamage);
					SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Data::Multiplier, SkillDamageMultiplier);
					SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Coef::Atk, SkillData->AttackPowerCoefficient);
					SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Coef::M_Atk, SkillData->MagicPowerCoefficient);
					SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Coef::Def, SkillData->DefenseCoefficient);
					SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Coef::Hp, SkillData->MaxHealthCoefficient);
				}

				InstigatorASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
			}
		}
	}
}

void UGP_BlueprintLibrary::ApplyGameplayEffectAndEventToActors(AActor* Instigator, const TArray<AActor*>& TargetActors, TSubclassOf<UGameplayEffect> EffectClass, FGameplayTag EventTag, float EffectLevel, UGP_SkillData* SkillData, float DamageScale)
{
	if (EffectClass)
	{
		ApplyGameplayEffectToActors(Instigator, TargetActors, EffectClass, EffectLevel, SkillData, DamageScale);
	}

	if (EventTag.IsValid())
	{
		SendGameplayEventToActors(Instigator, TargetActors, EventTag);
	}
}

void UGP_BlueprintLibrary::ApplyAreaGameplayEffectAtLocation(AActor* Instigator, const FVector& Location, float Radius, TSubclassOf<UGameplayEffect> EffectClass, float EffectLevel, UGP_SkillData* SkillData, bool bDrawDebug)
{
	if (!IsValid(Instigator) || !IsValid(EffectClass)) return;

	const TArray<AActor*> HitActors = SphereOverlapActorsAtLocation(Instigator, Location, Radius, Instigator, bDrawDebug);
	ApplyGameplayEffectToActors(Instigator, HitActors, EffectClass, EffectLevel, SkillData);
}
