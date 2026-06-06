#include "Characters/GP_MatadorMageBossCharacter.h"

#include "AI/Data/EnemyBlackboardKeys.h"
#include "AbilitySystem/Abilities/Enemy/GP_MatadorBullPatternAbility.h"
#include "AbilitySystem/Abilities/Enemy/GP_MatadorGroggyAbility.h"
#include "AbilitySystemComponent.h"
#include "Actors/GP_BullChargeActor.h"
#include "Actors/GP_ChainEffectActor.h"
#include "Actors/GP_MatadorBossDecoyActor.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/GP_MatadorBossStateComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AGP_MatadorMageBossCharacter::AGP_MatadorMageBossCharacter()
{
	bIsBossEnemy = true;
	BossDisplayName = NSLOCTEXT("GPMatadorMageBoss", "BossDisplayName", "Matador Mage");

	MatadorStateComponent = CreateDefaultSubobject<UGP_MatadorBossStateComponent>(TEXT("MatadorStateComponent"));

	DecoyActorClass = AGP_MatadorBossDecoyActor::StaticClass();
	ChainEffectActorClass = AGP_ChainEffectActor::StaticClass();
	BullChargeActorClass = AGP_BullChargeActor::StaticClass();
	MatadorBullPatternAbilityClass = UGP_MatadorBullPatternAbility::StaticClass();
	MatadorGroggyAbilityClass = UGP_MatadorGroggyAbility::StaticClass();

	// SK_MaskMan is the requested prototype mesh for the native Matador boss and decoy.
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MaskManMeshFinder(TEXT("/Game/Characters/MaskMan/SK_MaskMan.SK_MaskMan"));
	if (MaskManMeshFinder.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(MaskManMeshFinder.Object);
	}
}

void AGP_MatadorMageBossCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(MatadorStateComponent))
	{
		MatadorStateComponent->OnChainStageChanged.AddUniqueDynamic(this, &ThisClass::HandleMatadorChainStageChanged);
		MatadorStateComponent->OnGroggyChanged.AddUniqueDynamic(this, &ThisClass::HandleMatadorGroggyChanged);
	}

	if (!HasAuthority())
	{
		return;
	}

	if (IsValid(MatadorStateComponent))
	{
		MatadorStateComponent->InitializeMatadorState(this);
	}

	GrantMatadorPatternAbilities();

	if (bAutoSpawnDecoyOnBeginPlay)
	{
		EnsureMatadorDecoy();
	}
}

void AGP_MatadorMageBossCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GroggyRecoveryTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

AGP_MatadorBossDecoyActor* AGP_MatadorMageBossCharacter::EnsureMatadorDecoy()
{
	if (!HasAuthority() || !IsValid(MatadorStateComponent))
	{
		return IsValid(MatadorStateComponent) ? Cast<AGP_MatadorBossDecoyActor>(MatadorStateComponent->GetDecoyActor()) : nullptr;
	}

	if (AGP_MatadorBossDecoyActor* ExistingDecoy = Cast<AGP_MatadorBossDecoyActor>(MatadorStateComponent->GetDecoyActor()))
	{
		if (IsValid(ExistingDecoy) && !ExistingDecoy->IsActorBeingDestroyed())
		{
			return ExistingDecoy;
		}
	}

	if (!*DecoyActorClass)
	{
		return nullptr;
	}

	AActor* TargetActor = ResolvePatternTarget(nullptr);
	const FVector SpawnLocation = ResolveDecoySpawnLocation(TargetActor);
	const FRotator SpawnRotation = ResolveFacingRotation(SpawnLocation, GetActorLocation());

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AGP_MatadorBossDecoyActor* DecoyActor = GetWorld()->SpawnActor<AGP_MatadorBossDecoyActor>(
		DecoyActorClass,
		SpawnLocation,
		SpawnRotation,
		SpawnParameters);

	if (IsValid(DecoyActor))
	{
		// The decoy exposes the boss ASC, so player damage stays in the GAS damage path.
		DecoyActor->InitializeDecoy(this, MatadorStateComponent);
		EnsureChainEffect();
	}

	return DecoyActor;
}

AGP_ChainEffectActor* AGP_MatadorMageBossCharacter::EnsureChainEffect()
{
	if (!HasAuthority() || !IsValid(MatadorStateComponent))
	{
		return IsValid(MatadorStateComponent) ? Cast<AGP_ChainEffectActor>(MatadorStateComponent->GetChainEffectActor()) : nullptr;
	}

	if (AGP_ChainEffectActor* ExistingChain = Cast<AGP_ChainEffectActor>(MatadorStateComponent->GetChainEffectActor()))
	{
		if (IsValid(ExistingChain) && !ExistingChain->IsActorBeingDestroyed())
		{
			return ExistingChain;
		}
	}

	AGP_MatadorBossDecoyActor* DecoyActor = Cast<AGP_MatadorBossDecoyActor>(MatadorStateComponent->GetDecoyActor());
	if (!IsValid(DecoyActor) || !*ChainEffectActorClass)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AGP_ChainEffectActor* ChainActor = GetWorld()->SpawnActor<AGP_ChainEffectActor>(
		ChainEffectActorClass,
		GetActorLocation(),
		FRotator::ZeroRotator,
		SpawnParameters);

	if (IsValid(ChainActor))
	{
		// Chain stage mirrors the state component; VFX Blueprints only need to react to this actor.
		ChainActor->InitializeChain(this, DecoyActor, MatadorStateComponent);
	}

	return ChainActor;
}

AGP_BullChargeActor* AGP_MatadorMageBossCharacter::SpawnBullPattern(AActor* PatternTargetActor)
{
	if (!HasAuthority() || !IsValid(MatadorStateComponent) || MatadorStateComponent->IsGroggy())
	{
		return nullptr;
	}

	if (AGP_BullChargeActor* ExistingBull = Cast<AGP_BullChargeActor>(MatadorStateComponent->GetActiveBullActor()))
	{
		if (IsValid(ExistingBull) && !ExistingBull->IsActorBeingDestroyed())
		{
			return ExistingBull;
		}
	}

	AGP_MatadorBossDecoyActor* DecoyActor = EnsureMatadorDecoy();
	if (!IsValid(DecoyActor) || !*BullChargeActorClass)
	{
		return nullptr;
	}

	AActor* TargetActor = ResolvePatternTarget(PatternTargetActor);
	if (!IsValid(TargetActor))
	{
		TargetActor = DecoyActor;
	}

	const FVector SpawnLocation = ResolveBullSpawnLocation(TargetActor);
	FVector ChargeDirection = (TargetActor->GetActorLocation() - SpawnLocation).GetSafeNormal2D();
	if (ChargeDirection.IsNearlyZero())
	{
		ChargeDirection = GetActorForwardVector().GetSafeNormal2D();
	}
	if (ChargeDirection.IsNearlyZero())
	{
		ChargeDirection = FVector::ForwardVector;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AGP_BullChargeActor* BullActor = GetWorld()->SpawnActor<AGP_BullChargeActor>(
		BullChargeActorClass,
		SpawnLocation,
		ChargeDirection.Rotation(),
		SpawnParameters);

	if (IsValid(BullActor))
	{
		// Bull charge owns collision and damage; the boss ability only requests the pattern result.
		BullActor->InitializeBullCharge(TargetActor, DecoyActor, MatadorStateComponent, ChargeDirection);
	}

	return BullActor;
}

bool AGP_MatadorMageBossCharacter::RequestStartBullPattern(AActor* PatternTargetActor)
{
	return IsValid(SpawnBullPattern(PatternTargetActor));
}

void AGP_MatadorMageBossCharacter::RequestEnterGroggy()
{
	if (HasAuthority() && IsValid(MatadorStateComponent))
	{
		MatadorStateComponent->EnterGroggy();
	}
}

void AGP_MatadorMageBossCharacter::RequestRecoverFromGroggy()
{
	if (HasAuthority() && IsValid(MatadorStateComponent))
	{
		MatadorStateComponent->RecoverFromGroggy();
	}
}

bool AGP_MatadorMageBossCharacter::IsBullPatternActive() const
{
	return IsValid(MatadorStateComponent) && IsValid(MatadorStateComponent->GetActiveBullActor());
}

bool AGP_MatadorMageBossCharacter::ShouldTeleportForMatador(float DistanceToTarget) const
{
	return FMath::Max(0.0f, DistanceToTarget) >= FMath::Max(0.0f, TeleportDistanceThreshold);
}

void AGP_MatadorMageBossCharacter::HandleMatadorChainStageChanged(int32 ChainBreakCount, int32 ChainBreakTarget)
{
	if (AGP_ChainEffectActor* ChainActor = IsValid(MatadorStateComponent) ? Cast<AGP_ChainEffectActor>(MatadorStateComponent->GetChainEffectActor()) : nullptr)
	{
		ChainActor->SetChainStage(ChainBreakCount);
	}
}

void AGP_MatadorMageBossCharacter::HandleMatadorGroggyChanged(bool bNewGroggy)
{
	if (!HasAuthority())
	{
		return;
	}

	if (bNewGroggy)
	{
		if (AActor* ActiveBullActor = IsValid(MatadorStateComponent) ? MatadorStateComponent->GetActiveBullActor() : nullptr)
		{
			// Groggy cancels any lingering bull so the opening is readable and damage stays unguarded.
			ActiveBullActor->Destroy();
			MatadorStateComponent->RegisterActiveBullActor(nullptr);
		}

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				GroggyRecoveryTimerHandle,
				this,
				&ThisClass::RequestRecoverFromGroggy,
				FMath::Max(0.0f, GroggyDuration),
				false);
		}
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GroggyRecoveryTimerHandle);
	}

	if (bAutoSpawnDecoyOnBeginPlay)
	{
		EnsureMatadorDecoy();
	}
}

void AGP_MatadorMageBossCharacter::GrantMatadorPatternAbilities()
{
	if (!HasAuthority() || !bGrantMatadorPatternAbilities)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!IsValid(ASC))
	{
		return;
	}

	const TSubclassOf<UGameplayAbility> MatadorAbilities[] =
	{
		MatadorBullPatternAbilityClass,
		MatadorGroggyAbilityClass,
	};

	for (const TSubclassOf<UGameplayAbility>& AbilityClass : MatadorAbilities)
	{
		if (!*AbilityClass || ASC->FindAbilitySpecFromClass(AbilityClass) != nullptr)
		{
			continue;
		}

		// Native grants keep the GAS selector usable before a Blueprint child supplies custom abilities.
		ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass));
	}
}

AActor* AGP_MatadorMageBossCharacter::ResolvePatternTarget(AActor* ExplicitTargetActor) const
{
	if (IsValid(ExplicitTargetActor))
	{
		return ExplicitTargetActor;
	}

	if (const AAIController* AIController = Cast<AAIController>(GetController()))
	{
		if (const UBlackboardComponent* BlackboardComponent = AIController->GetBlackboardComponent())
		{
			return Cast<AActor>(BlackboardComponent->GetValueAsObject(EnemyBlackboardKeys::TargetActor));
		}
	}

	return nullptr;
}

FVector AGP_MatadorMageBossCharacter::ResolveDecoySpawnLocation(AActor* TargetActor) const
{
	FVector Forward = IsValid(TargetActor)
		? (TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal2D()
		: GetActorForwardVector().GetSafeNormal2D();
	if (Forward.IsNearlyZero())
	{
		Forward = FVector::ForwardVector;
	}

	const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward).GetSafeNormal();
	FVector DesiredLocation = GetActorLocation() + Forward * DecoyForwardOffset + Right * DecoySideOffset;

	if (UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld()))
	{
		FNavLocation ProjectedLocation;
		// Project prototype spawns to navmesh so the chain/decoy are usable in existing boss arenas.
		if (NavigationSystem->ProjectPointToNavigation(DesiredLocation, ProjectedLocation, FVector(220.0f, 220.0f, 280.0f)))
		{
			DesiredLocation = ProjectedLocation.Location;
		}
	}

	return DesiredLocation;
}

FVector AGP_MatadorMageBossCharacter::ResolveBullSpawnLocation(AActor* TargetActor) const
{
	const FVector TargetLocation = IsValid(TargetActor) ? TargetActor->GetActorLocation() : GetActorLocation() + GetActorForwardVector() * PreferredAirRange;
	FVector FromTargetToBoss = (GetActorLocation() - TargetLocation).GetSafeNormal2D();
	if (FromTargetToBoss.IsNearlyZero())
	{
		FromTargetToBoss = -GetActorForwardVector().GetSafeNormal2D();
	}
	if (FromTargetToBoss.IsNearlyZero())
	{
		FromTargetToBoss = -FVector::ForwardVector;
	}

	FVector DesiredLocation = TargetLocation + FromTargetToBoss * BullSpawnDistanceFromTarget + FVector(0.0f, 0.0f, BullSpawnHeightOffset);

	if (UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld()))
	{
		FNavLocation ProjectedLocation;
		// The bull is a world actor, but starting on navmesh avoids immediate wall-blocks in prototype maps.
		if (NavigationSystem->ProjectPointToNavigation(DesiredLocation, ProjectedLocation, FVector(260.0f, 260.0f, 320.0f)))
		{
			DesiredLocation = ProjectedLocation.Location + FVector(0.0f, 0.0f, BullSpawnHeightOffset);
		}
	}

	return DesiredLocation;
}

FRotator AGP_MatadorMageBossCharacter::ResolveFacingRotation(const FVector& FromLocation, const FVector& ToLocation) const
{
	FVector LookDirection = (ToLocation - FromLocation).GetSafeNormal2D();
	if (LookDirection.IsNearlyZero())
	{
		LookDirection = GetActorForwardVector().GetSafeNormal2D();
	}
	return LookDirection.IsNearlyZero() ? FRotator::ZeroRotator : LookDirection.Rotation();
}
