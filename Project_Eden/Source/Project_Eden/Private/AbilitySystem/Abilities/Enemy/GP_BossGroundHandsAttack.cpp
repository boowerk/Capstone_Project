#include "AbilitySystem/Abilities/Enemy/GP_BossGroundHandsAttack.h"

#include "AIController.h"
#include "AI/Data/EnemyBlackboardKeys.h"
#include "AI/Debug/EnemyAIDebugUtils.h"
#include "Actors/GP_BossGroundHandActor.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameplayTags/GP_Tags.h"

UGP_BossGroundHandsAttack::UGP_BossGroundHandsAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// The shared boss selector activates this pattern through its dedicated GAS asset tag.
	FGameplayTagContainer AbilityAssetTags;
	AbilityAssetTags.AddTag(GPTags::Ability::Enemy::Attack_BossGroundHands);
	SetAssetTags(AbilityAssetTags);

	GroundHandActorClass = AGP_BossGroundHandActor::StaticClass();
}

bool UGP_BossGroundHandsAttack::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const AActor* AvatarActor = ActorInfo != nullptr ? ActorInfo->AvatarActor.Get() : nullptr;
	const UWorld* World = IsValid(AvatarActor) ? AvatarActor->GetWorld() : nullptr;
	// Keep this long multi-wave pattern from winning every selector pass while still allowing other GAS candidates to run.
	return World == nullptr || World->GetTimeSeconds() - LastPatternStartTime >= FMath::Max(0.0f, PatternCooldown);
}

void UGP_BossGroundHandsAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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
	AActor* TargetActor = ResolvePatternTarget(AvatarActor);
	UWorld* World = IsValid(AvatarActor) ? AvatarActor->GetWorld() : nullptr;
	if (!HasAuthority(&ActivationInfo) || !IsValid(AvatarActor) || !IsValid(TargetActor) || !World || !GroundHandActorClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (APawn* AvatarPawn = Cast<APawn>(AvatarActor))
	{
		if (AAIController* AIController = Cast<AAIController>(AvatarPawn->GetController()))
		{
			// Sans commits to the cast while the floor warnings play instead of sliding through the pattern.
			AIController->StopMovement();
		}
	}

	LastPatternStartTime = World->GetTimeSeconds();
	PatternTargetActor = TargetActor;
	SpawnedHands.Reset();
	SpawnTimerHandles.Reset();

	const int32 SafeWaveCount = FMath::Max(1, WaveCount);
	const int32 SafeHandsPerWave = FMath::Max(1, HandsPerWave);
	for (int32 WaveIndex = 0; WaveIndex < SafeWaveCount; ++WaveIndex)
	{
		for (int32 HandIndex = 0; HandIndex < SafeHandsPerWave; ++HandIndex)
		{
			const float SpawnDelay = WaveIndex * FMath::Max(0.1f, WaveInterval)
				+ HandIndex * FMath::Max(0.0f, HandSpawnInterval);
			if (SpawnDelay <= KINDA_SMALL_NUMBER)
			{
				SpawnScheduledHand(WaveIndex, HandIndex);
				continue;
			}

			FTimerHandle& SpawnTimerHandle = SpawnTimerHandles.AddDefaulted_GetRef();
			FTimerDelegate SpawnDelegate = FTimerDelegate::CreateUObject(
				this,
				&ThisClass::SpawnScheduledHand,
				WaveIndex,
				HandIndex);
			World->GetTimerManager().SetTimer(SpawnTimerHandle, SpawnDelegate, SpawnDelay, false);
		}
	}

	const float LastSpawnDelay = (SafeWaveCount - 1) * FMath::Max(0.1f, WaveInterval)
		+ (SafeHandsPerWave - 1) * FMath::Max(0.0f, HandSpawnInterval);
	const float FinishDelay = LastSpawnDelay + FMath::Max(0.0f, TelegraphDuration)
		+ FMath::Max(0.1f, PatternFinishPadding);
	World->GetTimerManager().SetTimer(
		PatternFinishTimerHandle,
		this,
		&ThisClass::FinishPattern,
		FinishDelay,
		false);

	UE_LOG(
		LogEnemyAI,
		Log,
		TEXT("[BossAI] Sans ground hands pattern started. Boss=%s Target=%s Waves=%d HandsPerWave=%d Telegraph=%.2f"),
		*GetNameSafe(AvatarActor),
		*EnemyAIDebugUtils::DescribeActor(TargetActor),
		SafeWaveCount,
		SafeHandsPerWave,
		TelegraphDuration);
}

void UGP_BossGroundHandsAttack::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		for (FTimerHandle& SpawnTimerHandle : SpawnTimerHandles)
		{
			World->GetTimerManager().ClearTimer(SpawnTimerHandle);
		}
		World->GetTimerManager().ClearTimer(PatternFinishTimerHandle);
	}

	if (bWasCancelled)
	{
		for (const TWeakObjectPtr<AGP_BossGroundHandActor>& SpawnedHand : SpawnedHands)
		{
			if (SpawnedHand.IsValid())
			{
				SpawnedHand->Destroy();
			}
		}
	}

	SpawnTimerHandles.Reset();
	SpawnedHands.Reset();
	PatternTargetActor.Reset();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

AActor* UGP_BossGroundHandsAttack::ResolvePatternTarget(AActor* AvatarActor) const
{
	const APawn* AvatarPawn = Cast<APawn>(AvatarActor);
	const AAIController* AIController = IsValid(AvatarPawn) ? Cast<AAIController>(AvatarPawn->GetController()) : nullptr;
	const UBlackboardComponent* BlackboardComponent = IsValid(AIController) ? AIController->GetBlackboardComponent() : nullptr;
	return IsValid(BlackboardComponent)
		? Cast<AActor>(BlackboardComponent->GetValueAsObject(EnemyBlackboardKeys::TargetActor))
		: nullptr;
}

void UGP_BossGroundHandsAttack::SpawnScheduledHand(int32 WaveIndex, int32 HandIndex)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	AActor* TargetActor = PatternTargetActor.Get();
	UWorld* World = IsValid(AvatarActor) ? AvatarActor->GetWorld() : nullptr;
	if (!HasAuthority(&CurrentActivationInfo) || !IsValid(TargetActor) || !World || !GroundHandActorClass)
	{
		return;
	}

	FVector FormationForward = (TargetActor->GetActorLocation() - AvatarActor->GetActorLocation()).GetSafeNormal2D();
	if (FormationForward.IsNearlyZero())
	{
		FormationForward = AvatarActor->GetActorForwardVector().GetSafeNormal2D();
	}

	// Rotate each three-hand line so repeated waves cover different dodge directions around the player's current floor position.
	FormationForward = FRotator(0.0f, WaveIndex * 55.0f, 0.0f).RotateVector(FormationForward);
	const FVector FormationRight = FVector::CrossProduct(FVector::UpVector, FormationForward).GetSafeNormal();
	const float CenteredHandIndex = static_cast<float>(HandIndex) - (FMath::Max(1, HandsPerWave) - 1) * 0.5f;
	const FVector DesiredLocation = TargetActor->GetActorLocation()
		+ FormationRight * CenteredHandIndex * FMath::Max(0.0f, HandSpacing);

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = AvatarActor;
	SpawnParameters.Instigator = Cast<APawn>(AvatarActor);
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AGP_BossGroundHandActor* GroundHand = World->SpawnActor<AGP_BossGroundHandActor>(
		GroundHandActorClass,
		DesiredLocation,
		FormationForward.Rotation(),
		SpawnParameters);
	if (IsValid(GroundHand))
	{
		GroundHand->InitializeGroundHand(TelegraphDuration);
		SpawnedHands.Add(GroundHand);
	}
}

void UGP_BossGroundHandsAttack::FinishPattern()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
