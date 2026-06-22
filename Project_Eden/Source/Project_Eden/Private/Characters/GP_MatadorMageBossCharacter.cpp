#include "Characters/GP_MatadorMageBossCharacter.h"

#include "AI/Data/EnemyBlackboardKeys.h"
#include "AbilitySystem/Abilities/Enemy/GP_MatadorBullPatternAbility.h"
#include "AbilitySystem/Abilities/Enemy/GP_MatadorGroggyAbility.h"
#include "AbilitySystem/Abilities/Enemy/GP_MatadorMeleeAbilities.h"
#include "AbilitySystemComponent.h"
#include "Actors/GP_BullChargeActor.h"
#include "Actors/GP_ChainEffectActor.h"
#include "Actors/GP_MatadorBossDecoyActor.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Characters/GP_MatadorBossStateComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "VFX/GP_BossTelegraphVFXComponent.h"

AGP_MatadorMageBossCharacter::AGP_MatadorMageBossCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bIsBossEnemy = true;
	BossDisplayName = NSLOCTEXT("GPMatadorMageBoss", "BossDisplayName", "Matador Mage");

	MatadorStateComponent = CreateDefaultSubobject<UGP_MatadorBossStateComponent>(TEXT("MatadorStateComponent"));
	// The component stays dormant until an attack path explicitly honors the designer toggle.
	BossTelegraphVFXComponent = CreateDefaultSubobject<UGP_BossTelegraphVFXComponent>(TEXT("BossTelegraphVFXComponent"));
	BossTelegraphVFXComponent->SetupAttachment(GetRootComponent());
	BossTelegraphVFXComponent->SetAutoActivate(false);

	DecoyActorClass = AGP_MatadorBossDecoyActor::StaticClass();
	ChainEffectActorClass = AGP_ChainEffectActor::StaticClass();
	BullChargeActorClass = AGP_BullChargeActor::StaticClass();
	MatadorBullPatternAbilityClass = UGP_MatadorBullPatternAbility::StaticClass();
	MatadorGroggyAbilityClass = UGP_MatadorGroggyAbility::StaticClass();
	MatadorRapierThrustAbilityClass = UGP_MatadorRapierThrustAbility::StaticClass();
	MatadorCapeGustAbilityClass = UGP_MatadorCapeGustAbility::StaticClass();

	static ConstructorHelpers::FObjectFinder<UBehaviorTree> BossBehaviorTreeFinder(TEXT("/Game/Characters/EnemyCharacter/BT/Boss/BT_Boss_Matador.BT_Boss_Matador"));
	if (BossBehaviorTreeFinder.Succeeded())
	{
		BehaviorTreeAssetOverride = BossBehaviorTreeFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UBlackboardData> BossBlackboardFinder(TEXT("/Game/Characters/EnemyCharacter/BT/Boss/BB_Boss_Matador.BB_Boss_Matador"));
	if (BossBlackboardFinder.Succeeded())
	{
		BlackboardAssetOverride = BossBlackboardFinder.Object;
	}

	// SK_MaskMan is the requested prototype mesh for the native Matador boss and decoy.
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MaskManMeshFinder(TEXT("/Game/Characters/MaskMan/SK_MaskMan.SK_MaskMan"));
	if (MaskManMeshFinder.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(MaskManMeshFinder.Object);
	}
}

void AGP_MatadorMageBossCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority() || !IsValid(MatadorStateComponent) || MatadorStateComponent->IsGroggy())
	{
		return;
	}

	StopMainBodyMovement();
	UpdateDecoyFollow(DeltaSeconds);
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

	if (bUseFallbackMatadorPatternLoop && FallbackPatternInterval > 0.0f)
	{
		GetWorldTimerManager().SetTimer(
			FallbackPatternTimerHandle,
			this,
			&ThisClass::HandleMatadorFallbackPatternTick,
			FMath::Max(0.1f, FallbackPatternInterval),
			true,
			FMath::Max(0.1f, FallbackPatternInterval * 0.5f));
	}
}

void AGP_MatadorMageBossCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GroggyRecoveryTimerHandle);
		World->GetTimerManager().ClearTimer(GroggyDecoyTeleportTimerHandle);
		World->GetTimerManager().ClearTimer(FallbackPatternTimerHandle);
		World->GetTimerManager().ClearTimer(BullTelegraphTimerHandle);
	}
	bBullPatternPending = false;
	PendingBullPatternTarget.Reset();

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

	AActor* TargetActor = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!IsValid(TargetActor))
	{
		TargetActor = ResolvePatternTarget(PatternTargetActor);
	}

	const FVector SpawnLocation = ResolveBullSpawnLocation(DecoyActor, TargetActor);
	FVector ChargeDirection = (DecoyActor->GetActorLocation() - SpawnLocation).GetSafeNormal2D();
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
		// Bull starts as a matador scene: offscreen bull -> decoy lure -> decoy redirects to player -> player redirects back.
		BullActor->InitializeBullCharge(DecoyActor, TargetActor, DecoyActor, MatadorStateComponent, ChargeDirection);
		const FString PlayerLocationString = IsValid(TargetActor) ? TargetActor->GetActorLocation().ToCompactString() : TEXT("Invalid");
		UE_LOG(LogTemp, Log, TEXT("[MatadorBull] Spawned toward decoy. Boss=%s Decoy=%s Player=%s Bull=%s Spawn=%s DecoyLoc=%s PlayerLoc=%s Direction=%s"),
			*GetNameSafe(this),
			*GetNameSafe(DecoyActor),
			*GetNameSafe(TargetActor),
			*GetNameSafe(BullActor),
			*SpawnLocation.ToCompactString(),
			*DecoyActor->GetActorLocation().ToCompactString(),
			*PlayerLocationString,
			*ChargeDirection.ToCompactString());
	}

	return BullActor;
}

bool AGP_MatadorMageBossCharacter::RequestStartBullPattern(AActor* PatternTargetActor)
{
	if (!HasAuthority() || bBullPatternPending || IsBullPatternActive()
		|| !IsValid(MatadorStateComponent) || MatadorStateComponent->IsGroggy())
	{
		return false;
	}

	const float TelegraphDelay = IsValid(BossTelegraphVFXComponent)
		? BossTelegraphVFXComponent->PlayEnabledTelegraph()
		: 0.0f;
	if (TelegraphDelay <= KINDA_SMALL_NUMBER)
	{
		return IsValid(SpawnBullPattern(PatternTargetActor));
	}

	// Reserve the bull pattern while the cue plays so BT and fallback requests cannot queue duplicate bulls.
	bBullPatternPending = true;
	PendingBullPatternTarget = ResolvePatternTarget(PatternTargetActor);
	GetWorldTimerManager().SetTimer(
		BullTelegraphTimerHandle,
		this,
		&ThisClass::ExecutePendingBullPattern,
		TelegraphDelay,
		false);
	return true;
}

void AGP_MatadorMageBossCharacter::ExecutePendingBullPattern()
{
	AActor* TargetActor = PendingBullPatternTarget.Get();
	bBullPatternPending = false;
	PendingBullPatternTarget.Reset();
	SpawnBullPattern(TargetActor);
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
	return bBullPatternPending
		|| (IsValid(MatadorStateComponent) && IsValid(MatadorStateComponent->GetActiveBullActor()));
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
		// Groggy interrupts any attack still waiting behind its boss-level telegraph.
		GetWorldTimerManager().ClearTimer(BullTelegraphTimerHandle);
		bBullPatternPending = false;
		PendingBullPatternTarget.Reset();
		bHasPendingGroggyDecoyTeleportLocation = false;
		if (bTeleportToDecoyOnGroggy)
		{
			if (AActor* DecoyActor = IsValid(MatadorStateComponent) ? MatadorStateComponent->GetDecoyActor() : nullptr)
			{
				PendingGroggyDecoyTeleportLocation = DecoyActor->GetActorLocation() + FVector(0.0f, 0.0f, GroggyDecoyTeleportHeightOffset);
				bHasPendingGroggyDecoyTeleportLocation = true;
			}
		}

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

			if (bHasPendingGroggyDecoyTeleportLocation)
			{
				World->GetTimerManager().SetTimer(
					GroggyDecoyTeleportTimerHandle,
					this,
					&ThisClass::TeleportToPendingGroggyDecoyLocation,
					FMath::Max(0.0f, GroggyDecoyTeleportDelay),
					false);
			}
		}
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GroggyRecoveryTimerHandle);
		World->GetTimerManager().ClearTimer(GroggyDecoyTeleportTimerHandle);
	}
	bHasPendingGroggyDecoyTeleportLocation = false;

	if (bAutoSpawnDecoyOnBeginPlay)
	{
		EnsureMatadorDecoy();
	}
}

void AGP_MatadorMageBossCharacter::TeleportToPendingGroggyDecoyLocation()
{
	if (!HasAuthority() || !bHasPendingGroggyDecoyTeleportLocation)
	{
		return;
	}

	FVector TeleportLocation = PendingGroggyDecoyTeleportLocation;
	if (UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld()))
	{
		FNavLocation ProjectedLocation;
		if (NavigationSystem->ProjectPointToNavigation(TeleportLocation, ProjectedLocation, FVector(220.0f, 220.0f, 320.0f)))
		{
			TeleportLocation = ProjectedLocation.Location + FVector(0.0f, 0.0f, GroggyDecoyTeleportHeightOffset);
		}
	}

	SetActorLocation(TeleportLocation, false, nullptr, ETeleportType::TeleportPhysics);
	bHasPendingGroggyDecoyTeleportLocation = false;

	UE_LOG(LogTemp, Log, TEXT("[MatadorAI] Groggy teleport to decoy location. Boss=%s Location=%s"),
		*GetNameSafe(this),
		*TeleportLocation.ToCompactString());
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
		MatadorRapierThrustAbilityClass,
		MatadorCapeGustAbilityClass,
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

void AGP_MatadorMageBossCharacter::HandleMatadorFallbackPatternTick()
{
	if (!HasAuthority() || !IsValid(MatadorStateComponent) || MatadorStateComponent->IsGroggy() || IsBullPatternActive())
	{
		return;
	}

	AActor* TargetActor = ResolvePatternTarget(nullptr);
	if (!IsValid(TargetActor))
	{
		return;
	}

	const float DistanceToTarget = FVector::Dist2D(GetActorLocation(), TargetActor->GetActorLocation());
	if (DistanceToTarget < FallbackBullMinRange || DistanceToTarget > FallbackBullMaxRange)
	{
		return;
	}

	if (AGP_BullChargeActor* BullActor = SpawnBullPattern(TargetActor))
	{
		UE_LOG(LogTemp, Log, TEXT("[MatadorAI] Fallback bull pattern spawned. Boss=%s Target=%s Distance=%.1f Bull=%s"),
			*GetNameSafe(this),
			*GetNameSafe(TargetActor),
			DistanceToTarget,
			*GetNameSafe(BullActor));
	}
}

void AGP_MatadorMageBossCharacter::StopMainBodyMovement() const
{
	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->StopMovement();
	}
}

void AGP_MatadorMageBossCharacter::UpdateDecoyFollow(float DeltaSeconds)
{
	if (!bDecoyFollowsTarget || IsBullPatternActive())
	{
		return;
	}

	AGP_MatadorBossDecoyActor* DecoyActor = IsValid(MatadorStateComponent)
		? Cast<AGP_MatadorBossDecoyActor>(MatadorStateComponent->GetActiveDecoyActor())
		: nullptr;
	AActor* TargetActor = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!IsValid(DecoyActor) || !IsValid(TargetActor))
	{
		return;
	}

	FVector FromTargetToDecoy = DecoyActor->GetActorLocation() - TargetActor->GetActorLocation();
	FromTargetToDecoy.Z = 0.0f;
	if (FromTargetToDecoy.IsNearlyZero())
	{
		FromTargetToDecoy = -TargetActor->GetActorForwardVector().GetSafeNormal2D();
	}
	if (FromTargetToDecoy.IsNearlyZero())
	{
		FromTargetToDecoy = -FVector::ForwardVector;
	}

	FVector DesiredLocation = TargetActor->GetActorLocation()
		+ FromTargetToDecoy.GetSafeNormal2D() * FMath::Max(0.0f, DecoyFollowDesiredDistance);
	if (UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld()))
	{
		FNavLocation ProjectedLocation;
		if (NavigationSystem->ProjectPointToNavigation(DesiredLocation, ProjectedLocation, FVector(220.0f, 220.0f, 280.0f)))
		{
			DesiredLocation = ProjectedLocation.Location + FVector(0.0f, 0.0f, DecoyCapsuleCenterHeight);
		}
	}
	const FVector NewLocation = FMath::VInterpConstantTo(
		DecoyActor->GetActorLocation(),
		DesiredLocation,
		FMath::Max(0.0f, DeltaSeconds),
		FMath::Max(0.0f, DecoyFollowSpeed));
	const FRotator NewRotation = ResolveFacingRotation(NewLocation, TargetActor->GetActorLocation());
	DecoyActor->SetActorLocationAndRotation(NewLocation, NewRotation, true, nullptr, ETeleportType::None);
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
			if (AActor* BlackboardTarget = Cast<AActor>(BlackboardComponent->GetValueAsObject(EnemyBlackboardKeys::TargetActor)))
			{
				return BlackboardTarget;
			}
		}
	}

	// Temporary safety net: Matador patterns must remain testable while the dedicated BT is rebuilt.
	return UGameplayStatics::GetPlayerPawn(this, 0);
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
			DesiredLocation = ProjectedLocation.Location + FVector(0.0f, 0.0f, DecoyCapsuleCenterHeight);
		}
	}

	return DesiredLocation;
}

FVector AGP_MatadorMageBossCharacter::ResolveBullSpawnLocation(AActor* DecoyActor, AActor* PlayerTargetActor) const
{
	const FVector DecoyLocation = IsValid(DecoyActor) ? DecoyActor->GetActorLocation() : GetActorLocation();
	const FVector PlayerLocation = IsValid(PlayerTargetActor) ? PlayerTargetActor->GetActorLocation() : GetActorLocation() + GetActorForwardVector() * PreferredAirRange;
	FVector FromDecoyToPlayer = (PlayerLocation - DecoyLocation).GetSafeNormal2D();
	if (FromDecoyToPlayer.IsNearlyZero())
	{
		FromDecoyToPlayer = GetActorForwardVector().GetSafeNormal2D();
	}
	if (FromDecoyToPlayer.IsNearlyZero())
	{
		FromDecoyToPlayer = FVector::ForwardVector;
	}

	const FVector SideDirection = FRotator(0.0f, 90.0f, 0.0f).RotateVector(FromDecoyToPlayer).GetSafeNormal2D();
	FVector DesiredLocation =
		DecoyLocation
		- FromDecoyToPlayer * BullSpawnDistanceFromTarget
		+ SideDirection * BullSpawnSideOffsetFromPlayerLine
		+ FVector(0.0f, 0.0f, BullSpawnHeightOffset);

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
