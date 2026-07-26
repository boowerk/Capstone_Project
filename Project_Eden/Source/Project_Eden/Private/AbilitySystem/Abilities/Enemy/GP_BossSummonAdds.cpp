#include "AbilitySystem/Abilities/Enemy/GP_BossSummonAdds.h"

#include "AI/Controllers/EnemyAIController.h"
#include "AbilitySystem/GP_AttributeSet.h"
#include "Characters/GP_EnemyCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Pawn.h"
#include "GameplayTags/GP_Tags.h"
#include "Navigation/GP_GroundPlacement.h"
#include "UObject/ConstructorHelpers.h"

namespace GPBossSummonAdds
{
	const FGPGroundPlacementSettings GroundSettings
	{
		FVector(220.0f, 220.0f, 260.0f),
		250.0f,
		8
	};
	constexpr float MaxFinalGroundRise = 50.0f;
	constexpr float MaxFinalHorizontalAdjustment = 150.0f;
	const FVector FinalValidationExtent(100.0f, 100.0f, 100.0f);
}

UGP_BossSummonAdds::UGP_BossSummonAdds()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// The selector activates this ability through the existing boss summon gameplay tag.
	FGameplayTagContainer AbilityAssetTags;
	AbilityAssetTags.AddTag(GPTags::Ability::Enemy::Utility_BossSummon);
	SetAssetTags(AbilityAssetTags);

	// Sans summons the maintained basic melee template instead of the removed legacy enemy asset.
	static ConstructorHelpers::FClassFinder<AGP_EnemyCharacter> MeleeEnemyFinder(
		TEXT("/Game/Characters/EnemyCharacter/Basic/BP_BasicEnemy_Melee"));
	if (MeleeEnemyFinder.Succeeded())
	{
		SummonedEnemyClass = MeleeEnemyFinder.Class;
	}
}

void UGP_BossSummonAdds::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!HasAuthority(&ActivationInfo) || !IsValid(AvatarActor) || !*SummonedEnemyClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	AGP_EnemyCharacter* Summoner = Cast<AGP_EnemyCharacter>(AvatarActor);
	if (IsValid(Summoner))
	{
		Summoner->OnEnemyDeathStarted.AddUniqueDynamic(
			this,
			&ThisClass::HandleSummonerDeath);
	}

	FVector SummonerGroundAnchor;
	if (!TryGetSummonerGroundAnchor(AvatarActor, SummonerGroundAnchor))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[BossAI] Boss summon pattern skipped: no safe ground anchor was available for '%s'."),
			*GetNameSafe(AvatarActor));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	const int32 AliveAdds = CountAliveSummonedAdds();
	const int32 SpawnBudget = FMath::Clamp(MaxAliveSummonedAdds - AliveAdds, 0, SummonCount);
	int32 SpawnedCount = 0;

	for (int32 SpawnIndex = 0; SpawnIndex < SpawnBudget; ++SpawnIndex)
	{
		if (TrySpawnSummonedAdd(
			AvatarActor,
			SummonerGroundAnchor,
			SpawnIndex,
			SpawnBudget))
		{
			++SpawnedCount;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[BossAI] Boss summon pattern spawned %d melee adds. Alive=%d Budget=%d"), SpawnedCount, AliveAdds + SpawnedCount, SpawnBudget);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

int32 UGP_BossSummonAdds::CountAliveSummonedAdds()
{
	int32 AliveCount = 0;
	for (int32 Index = SummonedAdds.Num() - 1; Index >= 0; --Index)
	{
		AGP_EnemyCharacter* SummonedAdd = SummonedAdds[Index].Get();
		if (!IsValid(SummonedAdd) || SummonedAdd->IsActorBeingDestroyed())
		{
			SummonedAdds.RemoveAtSwap(Index);
			continue;
		}

		if (SummonedAdd->IsDead())
		{
			SummonedAdds.RemoveAtSwap(Index);
			continue;
		}

		const UGP_AttributeSet* AttributeSet = Cast<UGP_AttributeSet>(SummonedAdd->GetAttributeSet());
		if (IsValid(AttributeSet) && AttributeSet->GetHealth() <= KINDA_SMALL_NUMBER)
		{
			// Dead adds can remain in the world briefly for hit/death presentation, but they should not block future summons.
			SummonedAdds.RemoveAtSwap(Index);
			continue;
		}

		++AliveCount;
	}

	return AliveCount;
}

bool UGP_BossSummonAdds::TryGetSummonerGroundAnchor(
	AActor* AvatarActor,
	FVector& OutGroundAnchor) const
{
	UWorld* World = IsValid(AvatarActor) ? AvatarActor->GetWorld() : nullptr;
	if (!World)
	{
		return false;
	}

	FVector DesiredGroundAnchor = AvatarActor->GetActorLocation();
	if (const AGP_EnemyCharacter* Summoner =
		Cast<AGP_EnemyCharacter>(AvatarActor))
	{
		if (const UCapsuleComponent* Capsule =
			Summoner->GetCapsuleComponent())
		{
			DesiredGroundAnchor.Z -= Capsule->GetScaledCapsuleHalfHeight();
		}
	}

	return GPGroundPlacement::TryProjectGroundAnchor(
		World,
		DesiredGroundAnchor,
		GPBossSummonAdds::GroundSettings,
		OutGroundAnchor);
}

bool UGP_BossSummonAdds::TrySpawnSummonedAdd(
	AActor* AvatarActor,
	const FVector& SummonerGroundAnchor,
	int32 SpawnIndex,
	int32 SpawnTotal)
{
	UWorld* World = IsValid(AvatarActor) ? AvatarActor->GetWorld() : nullptr;
	if (!World)
	{
		return false;
	}

	const FRotator BossRotation(0.0f, AvatarActor->GetActorRotation().Yaw, 0.0f);
	const float AngleStep = SpawnTotal > 1 ? 360.0f / static_cast<float>(SpawnTotal) : 0.0f;
	const float BaseSpawnAngle =
		BossRotation.Yaw + 90.0f + AngleStep * SpawnIndex;
	const FVector ForwardOffset = BossRotation.Vector() * SpawnForwardOffset;
	const int32 PlacementAttempts =
		FMath::Max(1, GPBossSummonAdds::GroundSettings.MaxAttempts);
	for (int32 AttemptIndex = 0;
		AttemptIndex < PlacementAttempts;
		++AttemptIndex)
	{
		const int32 OffsetStep = (AttemptIndex + 1) / 2;
		const float OffsetSign =
			(AttemptIndex & 1) != 0 ? 1.0f : -1.0f;
		const float AngleOffset =
			AttemptIndex == 0
				? 0.0f
				: OffsetSign * OffsetStep * 22.5f;
		const float RadiusScale =
			AttemptIndex < 5 ? 1.0f : 0.65f;
		const float SpawnAngle = BaseSpawnAngle + AngleOffset;
		const FVector RingOffset =
			FRotationMatrix(FRotator(0.0f, SpawnAngle, 0.0f))
				.GetUnitAxis(EAxis::X)
			* SpawnRadius
			* RadiusScale;
		const FVector DesiredLocation =
			SummonerGroundAnchor + ForwardOffset + RingOffset;

		FVector SafeGroundLocation;
		if (!GPGroundPlacement::TryProjectReachableGround(
				World,
				SummonerGroundAnchor,
				DesiredLocation,
				GPBossSummonAdds::GroundSettings,
				SafeGroundLocation))
		{
			continue;
		}

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = AvatarActor;
		SpawnParameters.Instigator = Cast<APawn>(AvatarActor);
		SpawnParameters.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AGP_EnemyCharacter* SummonedAdd =
			World->SpawnActor<AGP_EnemyCharacter>(
				SummonedEnemyClass,
				SafeGroundLocation,
				BossRotation,
				SpawnParameters);
		if (!IsValid(SummonedAdd))
		{
			continue;
		}

		FVector ActualGroundLocation = SummonedAdd->GetActorLocation();
		const UCapsuleComponent* SummonedCapsule =
			SummonedAdd->GetCapsuleComponent();
		if (IsValid(SummonedCapsule))
		{
			ActualGroundLocation.Z -=
				SummonedCapsule->GetScaledCapsuleHalfHeight();
		}

		FGPGroundPlacementSettings FinalValidationSettings =
			GPBossSummonAdds::GroundSettings;
		FinalValidationSettings.ProjectionExtent =
			GPBossSummonAdds::FinalValidationExtent;
		FVector ReprojectedActualGround;
		const bool bFinalPlacementValid =
			IsValid(SummonedCapsule)
			&& GPGroundPlacement::IsGroundRiseWithinLimit(
				SafeGroundLocation,
				ActualGroundLocation,
				GPBossSummonAdds::MaxFinalGroundRise)
			&& FVector::DistSquared2D(
				SafeGroundLocation,
				ActualGroundLocation)
				<= FMath::Square(
					GPBossSummonAdds::MaxFinalHorizontalAdjustment)
			&& GPGroundPlacement::TryProjectReachableGround(
				World,
				SummonerGroundAnchor,
				ActualGroundLocation,
				FinalValidationSettings,
				ReprojectedActualGround)
			&& FVector::DistSquared2D(
				ActualGroundLocation,
				ReprojectedActualGround)
				<= FMath::Square(
					GPBossSummonAdds::FinalValidationExtent.X);
		if (!bFinalPlacementValid)
		{
			SummonedAdd->Destroy();
			continue;
		}

		SummonedAdd->SpawnDefaultController();
		if (AEnemyAIController* EnemyAIController =
			Cast<AEnemyAIController>(SummonedAdd->GetController()))
		{
			// Perception owns TargetActor, so request a rescore right after the
			// summoned melee enemy is possessed.
			EnemyAIController->RequestTargetActorReevaluation();
		}

		SummonedAdds.Add(SummonedAdd);
		return true;
	}

	return false;
}

void UGP_BossSummonAdds::HandleSummonerDeath(
	AGP_EnemyCharacter* Summoner,
	AActor* DeathInstigator)
{
	for (const TWeakObjectPtr<AGP_EnemyCharacter>& SummonedAddPtr :
		SummonedAdds)
	{
		AGP_EnemyCharacter* SummonedAdd = SummonedAddPtr.Get();
		if (IsValid(SummonedAdd)
			&& !SummonedAdd->IsActorBeingDestroyed()
			&& !SummonedAdd->IsDead())
		{
			// Preserve the enemy's ordinary terminal death/presentation path.
			// Summoned adds intentionally remain outside zone-completion counts.
			SummonedAdd->RequestDeath(
				IsValid(DeathInstigator)
					? DeathInstigator
					: Summoner);
		}
	}
	SummonedAdds.Reset();
}
