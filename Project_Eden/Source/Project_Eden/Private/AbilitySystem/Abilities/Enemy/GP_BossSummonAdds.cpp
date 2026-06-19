#include "AbilitySystem/Abilities/Enemy/GP_BossSummonAdds.h"

#include "AI/Controllers/EnemyAIController.h"
#include "AbilitySystem/GP_AttributeSet.h"
#include "Characters/GP_EnemyCharacter.h"
#include "GameFramework/Pawn.h"
#include "GameplayTags/GP_Tags.h"
#include "NavigationSystem.h"
#include "UObject/ConstructorHelpers.h"

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

	const int32 AliveAdds = CountAliveSummonedAdds();
	const int32 SpawnBudget = FMath::Clamp(MaxAliveSummonedAdds - AliveAdds, 0, SummonCount);
	int32 SpawnedCount = 0;

	for (int32 SpawnIndex = 0; SpawnIndex < SpawnBudget; ++SpawnIndex)
	{
		if (TrySpawnSummonedAdd(AvatarActor, SpawnIndex, SpawnBudget))
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

bool UGP_BossSummonAdds::TrySpawnSummonedAdd(AActor* AvatarActor, int32 SpawnIndex, int32 SpawnTotal)
{
	UWorld* World = IsValid(AvatarActor) ? AvatarActor->GetWorld() : nullptr;
	if (!World)
	{
		return false;
	}

	const FVector BossLocation = AvatarActor->GetActorLocation();
	const FRotator BossRotation(0.0f, AvatarActor->GetActorRotation().Yaw, 0.0f);
	const float AngleStep = SpawnTotal > 1 ? 360.0f / static_cast<float>(SpawnTotal) : 0.0f;
	const float SpawnAngle = BossRotation.Yaw + 90.0f + AngleStep * SpawnIndex;
	const FVector RingOffset = FRotationMatrix(FRotator(0.0f, SpawnAngle, 0.0f)).GetUnitAxis(EAxis::X) * SpawnRadius;
	const FVector ForwardOffset = BossRotation.Vector() * SpawnForwardOffset;
	FVector DesiredLocation = BossLocation + ForwardOffset + RingOffset;

	if (UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(World))
	{
		FNavLocation ProjectedLocation;
		// Project the summon point to navmesh so melee adds can immediately chase using their shared BT.
		if (NavigationSystem->ProjectPointToNavigation(DesiredLocation, ProjectedLocation, FVector(220.0f, 220.0f, 260.0f)))
		{
			DesiredLocation = ProjectedLocation.Location;
		}
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = AvatarActor;
	SpawnParameters.Instigator = Cast<APawn>(AvatarActor);
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AGP_EnemyCharacter* SummonedAdd = World->SpawnActor<AGP_EnemyCharacter>(
		SummonedEnemyClass,
		DesiredLocation,
		BossRotation,
		SpawnParameters);

	if (!IsValid(SummonedAdd))
	{
		return false;
	}

	SummonedAdd->SpawnDefaultController();
	if (AEnemyAIController* EnemyAIController = Cast<AEnemyAIController>(SummonedAdd->GetController()))
	{
		// Perception owns TargetActor, so request a rescore right after the summoned melee enemy is possessed.
		EnemyAIController->RequestTargetActorReevaluation();
	}

	SummonedAdds.Add(SummonedAdd);
	return true;
}
