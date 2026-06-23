#include "AbilitySystem/Abilities/Enemy/GP_MatadorMeleeAbilities.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AI/Data/EnemyBlackboardKeys.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/GP_EnemyCharacter.h"
#include "Characters/GP_MatadorBossStateComponent.h"
#include "Characters/GP_MatadorMageBossCharacter.h"
#include "Actors/GP_MatadorBossDecoyActor.h"
#include "Components/MeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"
#include "GameplayTags/GP_Tags.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Utils/GP_BlueprintLibrary.h"
#include "VFX/GP_BossTelegraphVFXComponent.h"

namespace GPMatadorMelee
{
	constexpr float AimTickInterval = 1.0f / 60.0f;

	AActor* GetBlackboardTarget(AActor* AvatarActor)
	{
		const APawn* AvatarPawn = Cast<APawn>(AvatarActor);
		const AAIController* AIController = IsValid(AvatarPawn) ? Cast<AAIController>(AvatarPawn->GetController()) : nullptr;
		const UBlackboardComponent* BlackboardComponent = IsValid(AIController) ? AIController->GetBlackboardComponent() : nullptr;
		return IsValid(BlackboardComponent) ? Cast<AActor>(BlackboardComponent->GetValueAsObject(EnemyBlackboardKeys::TargetActor)) : nullptr;
	}

	void AddDamageSetByCallers(FGameplayEffectSpec& Spec, float BaseDamage, float ToughnessDamage, float AttackPowerCoefficient)
	{
		Spec.SetSetByCallerMagnitude(GPTags::Damage::Data::Base, FMath::Max(0.0f, BaseDamage));
		Spec.SetSetByCallerMagnitude(GPTags::Damage::Data::BaseSpell, 0.0f);
		Spec.SetSetByCallerMagnitude(GPTags::Damage::Data::ToughnessBase, FMath::Max(0.0f, ToughnessDamage));
		Spec.SetSetByCallerMagnitude(GPTags::Damage::Data::Multiplier, 1.0f);
		Spec.SetSetByCallerMagnitude(GPTags::Damage::Coef::Atk, FMath::Max(0.0f, AttackPowerCoefficient));
		Spec.SetSetByCallerMagnitude(GPTags::Damage::Coef::M_Atk, 0.0f);
		Spec.SetSetByCallerMagnitude(GPTags::Damage::Coef::Def, 0.0f);
		Spec.SetSetByCallerMagnitude(GPTags::Damage::Coef::Hp, 0.0f);
	}

	TArray<AActor*> ForwardArcOverlapFromDirection(
		AActor* AvatarActor,
		const FVector& LockedDirection,
		float Radius,
		float ForwardOffset,
		float ArcAngleDegrees,
		float ElevationOffset,
		bool bDrawDebug)
	{
		TArray<AActor*> ActorsHit;
		if (!IsValid(AvatarActor))
		{
			return ActorsHit;
		}

		UWorld* World = AvatarActor->GetWorld();
		if (!IsValid(World))
		{
			return ActorsHit;
		}

		FVector ForwardVector = LockedDirection.GetSafeNormal2D();
		if (ForwardVector.IsNearlyZero())
		{
			ForwardVector = AvatarActor->GetActorForwardVector().GetSafeNormal2D();
		}
		if (ForwardVector.IsNearlyZero())
		{
			ForwardVector = FVector::ForwardVector;
		}

		FCollisionQueryParams CollisionQueryParams;
		CollisionQueryParams.AddIgnoredActor(AvatarActor);

		FCollisionResponseParams CollisionResponseParams;
		CollisionResponseParams.CollisionResponse.SetAllChannels(ECR_Ignore);
		CollisionResponseParams.CollisionResponse.SetResponse(ECC_Pawn, ECR_Block);

		TArray<FOverlapResult> OverlapResults;
		const FVector HitBoxLocation = AvatarActor->GetActorLocation()
			+ (ForwardVector * ForwardOffset)
			+ FVector(0.0f, 0.0f, ElevationOffset);
		World->OverlapMultiByChannel(
			OverlapResults,
			HitBoxLocation,
			FQuat::Identity,
			ECC_Visibility,
			FCollisionShape::MakeSphere(FMath::Max(0.0f, Radius)),
			CollisionQueryParams,
			CollisionResponseParams);

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
			if ((DirectionToTarget.IsNearlyZero() || bFullCircle || FVector::DotProduct(ForwardVector, DirectionToTarget.GetSafeNormal()) >= CosThreshold)
				&& UGP_BlueprintLibrary::CanApplyCombatEffect(AvatarActor, HitActor))
			{
				ActorsHit.AddUnique(HitActor);
			}
		}

		if (bDrawDebug)
		{
			const FVector Origin = AvatarActor->GetActorLocation() + FVector(0.0f, 0.0f, ElevationOffset);
			const float DrawDistance = ForwardOffset + Radius;
			DrawDebugSphere(World, HitBoxLocation, Radius, 24, FColor::Orange, false, 3.0f, 0, 2.0f);
			DrawDebugLine(World, Origin, Origin + FRotator(0.0f, -HalfAngleDegrees, 0.0f).RotateVector(ForwardVector) * DrawDistance, FColor::Yellow, false, 3.0f, 0, 3.0f);
			DrawDebugLine(World, Origin, Origin + FRotator(0.0f, HalfAngleDegrees, 0.0f).RotateVector(ForwardVector) * DrawDistance, FColor::Yellow, false, 3.0f, 0, 3.0f);
		}

		return ActorsHit;
	}
}

UGP_MatadorMeleeAbilityBase::UGP_MatadorMeleeAbilityBase()
{
	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageEffectFinder(TEXT("/Game/GAS_Pattern/AbilitySystem/GameplayEffects/Damage/GE_PrimaryDamage"));
	if (DamageEffectFinder.Succeeded())
	{
		DamageEffectClass = DamageEffectFinder.Class;
	}

	static ConstructorHelpers::FClassFinder<UGameplayEffect> CooldownEffectFinder(TEXT("/Game/GAS_Pattern/AbilitySystem/GameplayEffects/Cooldown/GE_Cooldown_Generic"));
	if (CooldownEffectFinder.Succeeded())
	{
		GenericCooldownEffectClass = CooldownEffectFinder.Class;
	}
}

bool UGP_MatadorMeleeAbilityBase::CheckCooldown(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CheckCooldown(Handle, ActorInfo, OptionalRelevantTags))
	{
		return false;
	}

	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (IsValid(ASC) && ASC->HasMatchingGameplayTag(GPTags::State::Status::Enemy::MatadorMeleeActive))
	{
		if (OptionalRelevantTags)
		{
			OptionalRelevantTags->AddTag(GPTags::State::Status::Enemy::MatadorMeleeActive);
		}
		return false;
	}

	const AActor* AvatarActor = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	const AGP_MatadorMageBossCharacter* MatadorBoss = Cast<AGP_MatadorMageBossCharacter>(AvatarActor);
	const UGP_MatadorBossStateComponent* MatadorStateComponent = IsValid(AvatarActor)
		? AvatarActor->FindComponentByClass<UGP_MatadorBossStateComponent>()
		: nullptr;
	if ((IsValid(MatadorBoss) && MatadorBoss->IsBullPatternActive())
		|| (!IsValid(MatadorBoss) && IsValid(MatadorStateComponent) && IsValid(MatadorStateComponent->GetActiveBullActor())))
	{
		// Treat the pre-spawn VFX delay as active combat state for melee cooldown gating.
		if (OptionalRelevantTags)
		{
			OptionalRelevantTags->AddTag(GPTags::Ability::Enemy::Utility_MatadorBullPattern);
		}
		return false;
	}

	if (IsValid(ASC) && CooldownTag.IsValid() && ASC->HasMatchingGameplayTag(CooldownTag))
	{
		if (OptionalRelevantTags)
		{
			OptionalRelevantTags->AddTag(CooldownTag);
		}
		return false;
	}

	return true;
}

void UGP_MatadorMeleeAbilityBase::ApplyCooldown(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	Super::ApplyCooldown(Handle, ActorInfo, ActivationInfo);

	if (!CooldownTag.IsValid() || CooldownDuration <= 0.0f || !GenericCooldownEffectClass)
	{
		return;
	}

	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(GenericCooldownEffectClass, GetAbilityLevel(Handle, ActorInfo));
	if (!SpecHandle.IsValid())
	{
		return;
	}

	SpecHandle.Data->DynamicGrantedTags.AddTag(CooldownTag);
	SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Cooldown::Data::Duration, CooldownDuration);
	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
}

void UGP_MatadorMeleeAbilityBase::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	ClearOwnedTimers();

	if (bMeleeLockActive)
	{
		if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
		{
			ASC->RemoveLooseGameplayTag(GPTags::State::Status::Enemy::MatadorMeleeActive);
		}
		bMeleeLockActive = false;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

AActor* UGP_MatadorMeleeAbilityBase::ResolveTargetActor(AActor* AvatarActor) const
{
	if (AActor* BlackboardTarget = GPMatadorMelee::GetBlackboardTarget(AvatarActor))
	{
		return BlackboardTarget;
	}

	const APawn* AvatarPawn = Cast<APawn>(AvatarActor);
	if (const AAIController* AIController = IsValid(AvatarPawn) ? Cast<AAIController>(AvatarPawn->GetController()) : nullptr)
	{
		if (AActor* FocusActor = AIController->GetFocusActor())
		{
			return FocusActor;
		}
	}

	return UGameplayStatics::GetPlayerPawn(this, 0);
}

AActor* UGP_MatadorMeleeAbilityBase::ResolvePatternActor(AActor* AbilityAvatarActor) const
{
	if (!IsValid(AbilityAvatarActor))
	{
		return nullptr;
	}

	if (AbilityAvatarActor->IsA<AGP_MatadorBossDecoyActor>())
	{
		return AbilityAvatarActor;
	}

	const UGP_MatadorBossStateComponent* MatadorStateComponent = AbilityAvatarActor->FindComponentByClass<UGP_MatadorBossStateComponent>();
	AActor* DecoyActor = IsValid(MatadorStateComponent) ? MatadorStateComponent->GetActiveDecoyActor() : nullptr;
	if (IsValid(DecoyActor) && !DecoyActor->IsActorBeingDestroyed())
	{
		return DecoyActor;
	}

	return AbilityAvatarActor;
}

bool UGP_MatadorMeleeAbilityBase::RotateAvatarTowardTarget(AActor* AvatarActor, AActor* TargetActor, float DeltaSeconds, float TurnRateDegreesPerSecond) const
{
	if (!IsValid(AvatarActor) || !IsValid(TargetActor))
	{
		return false;
	}

	FVector Direction = TargetActor->GetActorLocation() - AvatarActor->GetActorLocation();
	Direction.Z = 0.0f;
	if (Direction.IsNearlyZero())
	{
		return false;
	}

	const FRotator CurrentRotation = AvatarActor->GetActorRotation();
	const FRotator TargetRotation = Direction.Rotation();
	const FRotator NewRotation = FMath::RInterpConstantTo(CurrentRotation, TargetRotation, DeltaSeconds, FMath::Max(1.0f, TurnRateDegreesPerSecond));
	AvatarActor->SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
	return true;
}

void UGP_MatadorMeleeAbilityBase::StopAIMovement(AActor* AvatarActor) const
{
	if (APawn* AvatarPawn = Cast<APawn>(AvatarActor))
	{
		if (AAIController* AIController = Cast<AAIController>(AvatarPawn->GetController()))
		{
			AIController->StopMovement();
		}
	}
}

void UGP_MatadorMeleeAbilityBase::PlayConfiguredMontage()
{
	if (!SkillMontage)
	{
		return;
	}

	if (UAbilityTask_PlayMontageAndWait* PlayMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, SkillMontage, 1.0f))
	{
		PlayMontageTask->ReadyForActivation();
	}
}

void UGP_MatadorMeleeAbilityBase::ApplySetByCallerDamage(AActor* AvatarActor, const TArray<AActor*>& TargetActors, float BaseDamage, float ToughnessDamage, float AttackPowerCoefficient) const
{
	if (!HasAuthority(&CurrentActivationInfo) || !IsValid(AvatarActor) || !DamageEffectClass)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(AvatarActor);
	if (!IsValid(SourceASC))
	{
		return;
	}

	for (AActor* TargetActor : TargetActors)
	{
		if (!UGP_BlueprintLibrary::CanApplyCombatEffect(AvatarActor, TargetActor))
		{
			continue;
		}

		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
		if (!IsValid(TargetASC))
		{
			continue;
		}

		FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
		ContextHandle.AddInstigator(AvatarActor, AvatarActor);
		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, GetAbilityLevel(), ContextHandle);
		if (!SpecHandle.IsValid())
		{
			continue;
		}

		GPMatadorMelee::AddDamageSetByCallers(*SpecHandle.Data.Get(), BaseDamage, ToughnessDamage, AttackPowerCoefficient);
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}
}

void UGP_MatadorMeleeAbilityBase::ApplyEffectToTargets(AActor* AvatarActor, const TArray<AActor*>& TargetActors, TSubclassOf<UGameplayEffect> EffectClass) const
{
	if (!HasAuthority(&CurrentActivationInfo) || !IsValid(AvatarActor) || !EffectClass)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(AvatarActor);
	if (!IsValid(SourceASC))
	{
		return;
	}

	for (AActor* TargetActor : TargetActors)
	{
		if (!UGP_BlueprintLibrary::CanApplyCombatEffect(AvatarActor, TargetActor))
		{
			continue;
		}

		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
		if (!IsValid(TargetASC))
		{
			continue;
		}

		FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
		ContextHandle.AddInstigator(AvatarActor, AvatarActor);
		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(EffectClass, GetAbilityLevel(), ContextHandle);
		if (SpecHandle.IsValid())
		{
			SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		}
	}
}

float UGP_MatadorMeleeAbilityBase::ResolveBlackboardHealthRatio(AActor* AvatarActor) const
{
	const APawn* AvatarPawn = Cast<APawn>(AvatarActor);
	const AAIController* AIController = IsValid(AvatarPawn) ? Cast<AAIController>(AvatarPawn->GetController()) : nullptr;
	const UBlackboardComponent* BlackboardComponent = IsValid(AIController) ? AIController->GetBlackboardComponent() : nullptr;
	if (IsValid(BlackboardComponent) && BlackboardComponent->GetKeyID(EnemyBlackboardKeys::HealthRatio) != FBlackboard::InvalidKey)
	{
		return FMath::Clamp(BlackboardComponent->GetValueAsFloat(EnemyBlackboardKeys::HealthRatio), 0.0f, 1.0f);
	}

	return 1.0f;
}

bool UGP_MatadorMeleeAbilityBase::TryStartMeleeLock(const FGameplayAbilityActorInfo* ActorInfo)
{
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!IsValid(ASC) || ASC->HasMatchingGameplayTag(GPTags::State::Status::Enemy::MatadorMeleeActive))
	{
		return false;
	}

	ASC->AddLooseGameplayTag(GPTags::State::Status::Enemy::MatadorMeleeActive);
	bMeleeLockActive = true;
	return true;
}

void UGP_MatadorMeleeAbilityBase::ClearOwnedTimers()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PrimaryTimerHandle);
		World->GetTimerManager().ClearTimer(SecondaryTimerHandle);
		World->GetTimerManager().ClearTimer(TertiaryTimerHandle);
	}
}

UGP_MatadorRapierThrustAbility::UGP_MatadorRapierThrustAbility()
{
	FGameplayTagContainer AbilityAssetTags;
	AbilityAssetTags.AddTag(GPTags::Ability::Boss::Matador::RapierThrust);
	SetAssetTags(AbilityAssetTags);

	CooldownTag = GPTags::Cooldown::Enemy::Matador::RapierThrust;
	CooldownDuration = 8.0f;
	FallbackBaseDamage = 26.0f;
	FallbackToughnessDamage = 10.0f;
	FallbackAttackPowerCoefficient = 1.1f;
	ThrustRange = 7500.0f;
}

void UGP_MatadorRapierThrustAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (!TryStartMeleeLock(ActorInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* AvatarActor = ResolvePatternActor(GetAvatarActorFromActorInfo());
	if (!IsValid(AvatarActor))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	StopAIMovement(AvatarActor);
	AGP_MatadorMageBossCharacter* MatadorBoss = Cast<AGP_MatadorMageBossCharacter>(AvatarActor);
	const float TelegraphDelay = IsValid(MatadorBoss)
		? MatadorBoss->PlayBossTelegraphForPattern(GPTags::Ability::Boss::Matador::RapierThrust)
		: 0.0f;
	if (TelegraphDelay > KINDA_SMALL_NUMBER)
	{
		// The existing aim phase starts only after the optional boss-level Niagara cue has completed.
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(PrimaryTimerHandle, this, &ThisClass::BeginRapierPattern, TelegraphDelay, false);
			return;
		}
	}

	BeginRapierPattern();
}

void UGP_MatadorRapierThrustAbility::BeginRapierPattern()
{
	AActor* AvatarActor = ResolvePatternActor(GetAvatarActorFromActorInfo());
	if (!IsValid(AvatarActor))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	PlayConfiguredMontage();
	AimElapsedTime = 0.0f;
	LockedThrustDirection = AvatarActor->GetActorForwardVector().GetSafeNormal2D();
	SetRapierGlow(AvatarActor, 0.0f);
	BP_OnRapierAimStarted(AvatarActor, AimDuration);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(PrimaryTimerHandle, this, &ThisClass::TickAim, GPMatadorMelee::AimTickInterval, true);
	}
}

void UGP_MatadorRapierThrustAbility::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	SetRapierGlow(ResolvePatternActor(GetAvatarActorFromActorInfo()), 0.0f);
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGP_MatadorRapierThrustAbility::TickAim()
{
	AActor* AvatarActor = ResolvePatternActor(GetAvatarActorFromActorInfo());
	if (!IsValid(AvatarActor))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	StopAIMovement(AvatarActor);
	RotateAvatarTowardTarget(AvatarActor, ResolveTargetActor(AvatarActor), GPMatadorMelee::AimTickInterval, AimTurnRateDegreesPerSecond);

	AimElapsedTime += GPMatadorMelee::AimTickInterval;
	const float GlowAlpha = AimDuration > KINDA_SMALL_NUMBER ? FMath::Clamp(AimElapsedTime / AimDuration, 0.0f, 1.0f) : 1.0f;
	SetRapierGlow(AvatarActor, GlowAlpha);
	BP_OnRapierGlowUpdated(AvatarActor, GlowAlpha);

	if (AimElapsedTime >= AimDuration)
	{
		LockDirection();
		return;
	}

	FVector PreviewDirection = AvatarActor->GetActorForwardVector().GetSafeNormal2D();
	if (AActor* TargetActor = ResolveTargetActor(AvatarActor))
	{
		PreviewDirection = (TargetActor->GetActorLocation() - AvatarActor->GetActorLocation()).GetSafeNormal2D();
	}
	DrawRapierTelegraph(AvatarActor, PreviewDirection, RapierAimTelegraphColor, GPMatadorMelee::AimTickInterval * 1.5f);
}

void UGP_MatadorRapierThrustAbility::LockDirection()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PrimaryTimerHandle);
	}

	AActor* AvatarActor = ResolvePatternActor(GetAvatarActorFromActorInfo());
	if (!IsValid(AvatarActor))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	if (AActor* TargetActor = ResolveTargetActor(AvatarActor))
	{
		LockedThrustDirection = (TargetActor->GetActorLocation() - AvatarActor->GetActorLocation()).GetSafeNormal2D();
	}
	else
	{
		LockedThrustDirection = AvatarActor->GetActorForwardVector().GetSafeNormal2D();
	}
	if (LockedThrustDirection.IsNearlyZero())
	{
		LockedThrustDirection = FVector::ForwardVector;
	}

	BP_OnRapierDirectionLocked(AvatarActor, LockedThrustDirection, CommitDelay);
	DrawRapierTelegraph(AvatarActor, LockedThrustDirection, RapierLockedTelegraphColor, FMath::Max(0.05f, CommitDelay));

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(SecondaryTimerHandle, this, &ThisClass::PerformThrust, FMath::Max(0.0f, CommitDelay), false);
	}
}

void UGP_MatadorRapierThrustAbility::PerformThrust()
{
	AActor* AvatarActor = ResolvePatternActor(GetAvatarActorFromActorInfo());
	if (!IsValid(AvatarActor))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	const FVector BoxExtent(FMath::Max(1.0f, ThrustRange * 0.5f), FMath::Max(1.0f, ThrustHalfWidth), FMath::Max(1.0f, ThrustHalfHeight));
	const FVector BoxCenter = AvatarActor->GetActorLocation()
		+ LockedThrustDirection * FMath::Max(1.0f, ThrustRange * 0.5f)
		+ FVector(0.0f, 0.0f, HitBoxElevationOffset);
	const FRotator BoxRotation = LockedThrustDirection.Rotation();
	DrawRapierTelegraph(AvatarActor, LockedThrustDirection, RapierLockedTelegraphColor, 0.15f);
	const TArray<AActor*> HitActors = UGP_BlueprintLibrary::BoxOverlapActorsAtLocation(
		AvatarActor,
		BoxCenter,
		BoxExtent,
		BoxRotation,
		AvatarActor,
		bDebugHitShape || bDrawDebugs);

	ApplySetByCallerDamage(AvatarActor, HitActors, FallbackBaseDamage, FallbackToughnessDamage, FallbackAttackPowerCoefficient);
	BP_OnRapierThrust(AvatarActor, LockedThrustDirection);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(TertiaryTimerHandle, this, &ThisClass::FinishAfterThrust, FMath::Max(0.0f, PostThrustRecovery), false);
	}
}

void UGP_MatadorRapierThrustAbility::FinishAfterThrust()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGP_MatadorRapierThrustAbility::SetRapierGlow(AActor* AvatarActor, float GlowAlpha) const
{
	if (!IsValid(AvatarActor) || RapierGlowParameterName.IsNone())
	{
		return;
	}

	const float GlowValue = FMath::Clamp(GlowAlpha, 0.0f, 1.0f) * FMath::Max(0.0f, MaxRapierGlowValue);
	TArray<UMeshComponent*> MeshComponents;
	AvatarActor->GetComponents<UMeshComponent>(MeshComponents);
	for (UMeshComponent* MeshComponent : MeshComponents)
	{
		if (IsValid(MeshComponent))
		{
			MeshComponent->SetScalarParameterValueOnMaterials(RapierGlowParameterName, GlowValue);
		}
	}
}

void UGP_MatadorRapierThrustAbility::DrawRapierTelegraph(AActor* AvatarActor, const FVector& Direction, FColor Color, float Duration) const
{
	if (!bShowTemporaryTelegraph || !IsValid(AvatarActor))
	{
		return;
	}

	UWorld* World = AvatarActor->GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	FVector DrawDirection = Direction.GetSafeNormal2D();
	if (DrawDirection.IsNearlyZero())
	{
		DrawDirection = AvatarActor->GetActorForwardVector().GetSafeNormal2D();
	}
	if (DrawDirection.IsNearlyZero())
	{
		DrawDirection = FVector::ForwardVector;
	}

	const FVector BoxExtent(FMath::Max(1.0f, ThrustRange * 0.5f), FMath::Max(1.0f, ThrustHalfWidth), FMath::Max(1.0f, ThrustHalfHeight));
	const FVector BoxCenter = AvatarActor->GetActorLocation()
		+ DrawDirection * FMath::Max(1.0f, ThrustRange * 0.5f)
		+ FVector(0.0f, 0.0f, HitBoxElevationOffset);
	DrawDebugBox(World, BoxCenter, BoxExtent, DrawDirection.Rotation().Quaternion(), Color, false, Duration, 0, 4.0f);
}

UGP_MatadorCapeGustAbility::UGP_MatadorCapeGustAbility()
{
	FGameplayTagContainer AbilityAssetTags;
	AbilityAssetTags.AddTag(GPTags::Ability::Boss::Matador::CapeGust);
	SetAssetTags(AbilityAssetTags);

	CooldownTag = GPTags::Cooldown::Enemy::Matador::CapeGust;
	CooldownDuration = 5.0f;
	FallbackBaseDamage = 8.0f;
	FallbackToughnessDamage = 2.0f;
	FallbackAttackPowerCoefficient = 0.35f;
}

void UGP_MatadorCapeGustAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (!TryStartMeleeLock(ActorInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* AvatarActor = ResolvePatternActor(GetAvatarActorFromActorInfo());
	if (!IsValid(AvatarActor))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	StopAIMovement(AvatarActor);
	AGP_MatadorMageBossCharacter* MatadorBoss = Cast<AGP_MatadorMageBossCharacter>(AvatarActor);
	const float TelegraphDelay = IsValid(MatadorBoss)
		? MatadorBoss->PlayBossTelegraphForPattern(GPTags::Ability::Boss::Matador::CapeGust)
		: 0.0f;
	if (TelegraphDelay > KINDA_SMALL_NUMBER)
	{
		// Cape preparation remains unchanged, but it begins after the optional shared cue.
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(PrimaryTimerHandle, this, &ThisClass::BeginCapePattern, TelegraphDelay, false);
			return;
		}
	}

	BeginCapePattern();
}

void UGP_MatadorCapeGustAbility::BeginCapePattern()
{
	AActor* AvatarActor = ResolvePatternActor(GetAvatarActorFromActorInfo());
	if (!IsValid(AvatarActor))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	PlayConfiguredMontage();
	PrepareElapsedTime = 0.0f;
	LockedGustDirection = AvatarActor->GetActorForwardVector().GetSafeNormal2D();
	BurstIndex = 0;
	BurstCount = 1;
	BP_OnCapePrepareStarted(AvatarActor, PrepareDuration);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(PrimaryTimerHandle, this, &ThisClass::TickPrepare, GPMatadorMelee::AimTickInterval, true);
	}
}

void UGP_MatadorCapeGustAbility::TickPrepare()
{
	AActor* AvatarActor = ResolvePatternActor(GetAvatarActorFromActorInfo());
	if (!IsValid(AvatarActor))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	PrepareElapsedTime += GPMatadorMelee::AimTickInterval;
	if (PrepareElapsedTime >= PrepareDuration)
	{
		LockDirectionAndStartBursts();
		return;
	}

	FVector PreviewDirection = AvatarActor->GetActorForwardVector().GetSafeNormal2D();
	if (AActor* TargetActor = ResolveTargetActor(AvatarActor))
	{
		PreviewDirection = (TargetActor->GetActorLocation() - AvatarActor->GetActorLocation()).GetSafeNormal2D();
	}
	DrawCapeTelegraph(AvatarActor, PreviewDirection, CapePrepareTelegraphColor, GPMatadorMelee::AimTickInterval * 1.5f);
}

void UGP_MatadorCapeGustAbility::LockDirectionAndStartBursts()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PrimaryTimerHandle);
	}

	AActor* AvatarActor = ResolvePatternActor(GetAvatarActorFromActorInfo());
	if (!IsValid(AvatarActor))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	if (AActor* TargetActor = ResolveTargetActor(AvatarActor))
	{
		LockedGustDirection = (TargetActor->GetActorLocation() - AvatarActor->GetActorLocation()).GetSafeNormal2D();
	}
	else
	{
		LockedGustDirection = AvatarActor->GetActorForwardVector().GetSafeNormal2D();
	}
	if (LockedGustDirection.IsNearlyZero())
	{
		LockedGustDirection = FVector::ForwardVector;
	}

	const float HealthRatio = ResolveBlackboardHealthRatio(AvatarActor);
	BurstCount = HealthRatio <= LowHealthThreshold ? FMath::Max(1, LowHealthBurstCount) : 1;
	BurstIndex = 0;
	BP_OnCapeDirectionLocked(AvatarActor, LockedGustDirection);
	DrawCapeTelegraph(AvatarActor, LockedGustDirection, CapeBurstTelegraphColor, 0.2f);
	PerformGustBurst();
}

void UGP_MatadorCapeGustAbility::PerformGustBurst()
{
	AActor* AvatarActor = ResolvePatternActor(GetAvatarActorFromActorInfo());
	if (!IsValid(AvatarActor))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	DrawCapeTelegraph(AvatarActor, LockedGustDirection, CapeBurstTelegraphColor, 0.15f);
	const TArray<AActor*> HitActors = GPMatadorMelee::ForwardArcOverlapFromDirection(
		AvatarActor,
		LockedGustDirection,
		GustRange,
		GustForwardOffset,
		GustArcAngleDegrees,
		HitBoxElevationOffset,
		bDebugHitShape || bDrawDebugs);

	ApplySetByCallerDamage(AvatarActor, HitActors, FallbackBaseDamage, FallbackToughnessDamage, FallbackAttackPowerCoefficient);
	ApplyEffectToTargets(AvatarActor, HitActors, SlowEffectClass);
	ApplyKnockback(AvatarActor, HitActors);

	BP_OnCapeGustBurst(AvatarActor, LockedGustDirection, BurstIndex, BurstCount);
	++BurstIndex;

	if (BurstIndex < BurstCount)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(SecondaryTimerHandle, this, &ThisClass::PerformGustBurst, FMath::Max(0.01f, LowHealthBurstInterval), false);
		}
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(TertiaryTimerHandle, this, &ThisClass::FinishCapeGust, FMath::Max(0.0f, PostGustRecovery), false);
	}
}

void UGP_MatadorCapeGustAbility::FinishCapeGust()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGP_MatadorCapeGustAbility::DrawCapeTelegraph(AActor* AvatarActor, const FVector& Direction, FColor Color, float Duration) const
{
	if (!bShowTemporaryTelegraph || !IsValid(AvatarActor))
	{
		return;
	}

	UWorld* World = AvatarActor->GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	FVector ForwardVector = Direction.GetSafeNormal2D();
	if (ForwardVector.IsNearlyZero())
	{
		ForwardVector = AvatarActor->GetActorForwardVector().GetSafeNormal2D();
	}
	if (ForwardVector.IsNearlyZero())
	{
		ForwardVector = FVector::ForwardVector;
	}

	const float HalfAngleDegrees = FMath::Clamp(GustArcAngleDegrees * 0.5f, 0.0f, 180.0f);
	const FVector Origin = AvatarActor->GetActorLocation() + FVector(0.0f, 0.0f, HitBoxElevationOffset);
	const FVector Center = Origin + ForwardVector * GustForwardOffset;
	const float DrawDistance = GustForwardOffset + GustRange;
	const FVector LeftEdge = Origin + FRotator(0.0f, -HalfAngleDegrees, 0.0f).RotateVector(ForwardVector) * DrawDistance;
	const FVector RightEdge = Origin + FRotator(0.0f, HalfAngleDegrees, 0.0f).RotateVector(ForwardVector) * DrawDistance;
	const FVector CenterEdge = Origin + ForwardVector * DrawDistance;

	DrawDebugLine(World, Origin, LeftEdge, Color, false, Duration, 0, 5.0f);
	DrawDebugLine(World, Origin, RightEdge, Color, false, Duration, 0, 5.0f);
	DrawDebugLine(World, Origin, CenterEdge, Color, false, Duration, 0, 2.0f);
	DrawDebugSphere(World, Center, FMath::Max(1.0f, GustRange), 24, Color, false, Duration, 0, 2.0f);

	constexpr int32 SegmentCount = 8;
	FVector PreviousPoint = LeftEdge;
	for (int32 SegmentIndex = 1; SegmentIndex <= SegmentCount; ++SegmentIndex)
	{
		const float Alpha = static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
		const float YawOffset = FMath::Lerp(-HalfAngleDegrees, HalfAngleDegrees, Alpha);
		const FVector CurrentPoint = Origin + FRotator(0.0f, YawOffset, 0.0f).RotateVector(ForwardVector) * DrawDistance;
		DrawDebugLine(World, PreviousPoint, CurrentPoint, Color, false, Duration, 0, 4.0f);
		PreviousPoint = CurrentPoint;
	}
}

void UGP_MatadorCapeGustAbility::ApplyKnockback(AActor* AvatarActor, const TArray<AActor*>& HitActors) const
{
	if (!HasAuthority(&CurrentActivationInfo) || !IsValid(AvatarActor))
	{
		return;
	}

	const FVector KnockbackVelocity = LockedGustDirection * FMath::Max(0.0f, KnockbackStrength)
		+ FVector(0.0f, 0.0f, FMath::Max(0.0f, KnockbackUpwardStrength));

	for (AActor* HitActor : HitActors)
	{
		if (!UGP_BlueprintLibrary::CanApplyCombatEffect(AvatarActor, HitActor))
		{
			continue;
		}

		if (ACharacter* HitCharacter = Cast<ACharacter>(HitActor))
		{
			HitCharacter->LaunchCharacter(KnockbackVelocity, true, false);
		}
		else if (UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(HitActor->GetRootComponent()))
		{
			RootPrimitive->AddImpulse(KnockbackVelocity * RootPrimitive->GetMass(), NAME_None, true);
		}
	}
}
