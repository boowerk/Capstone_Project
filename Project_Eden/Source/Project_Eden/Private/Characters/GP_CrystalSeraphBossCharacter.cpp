#include "Characters/GP_CrystalSeraphBossCharacter.h"

#include "AbilitySystem/GP_AttributeSet.h"
#include "AbilitySystem/Abilities/Enemy/GP_CrystalSeraphPatternAbility.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "AIController.h"
#include "AI/Data/EnemyBlackboardKeys.h"
#include "Actors/GP_CrystalPrismActor.h"
#include "Actors/GP_CrystalSanctuaryMarkerActor.h"
#include "Actors/GP_CrystalShardProjectile.h"
#include "Actors/GP_SeraphLaserActor.h"
#include "Actors/GP_WingCoreHitActor.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Characters/GP_CrystalSeraphStateComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AGP_CrystalSeraphBossCharacter::AGP_CrystalSeraphBossCharacter()
{
	bIsBossEnemy = true;
	BossDisplayName = NSLOCTEXT("GPCrystalSeraphBoss", "BossDisplayName", "Crystal Seraph");

	CrystalSeraphStateComponent = CreateDefaultSubobject<UGP_CrystalSeraphStateComponent>(TEXT("CrystalSeraphStateComponent"));
	CrystalPrismActorClass = AGP_CrystalPrismActor::StaticClass();
	SeraphLaserActorClass = AGP_SeraphLaserActor::StaticClass();
	WingCoreHitActorClass = AGP_WingCoreHitActor::StaticClass();
	CrystalShardProjectileClass = AGP_CrystalShardProjectile::StaticClass();
	AreaMarkerActorClass = AGP_CrystalSanctuaryMarkerActor::StaticClass();
	CrystalShardAbilityClass = UGP_CrystalSeraphShardAbility::StaticClass();
	CrystalLaserAbilityClass = UGP_CrystalSeraphLaserAbility::StaticClass();
	CrystalPrismAbilityClass = UGP_CrystalSeraphPrismAbility::StaticClass();
	CrystalAreaAbilityClass = UGP_CrystalSeraphAreaAbility::StaticClass();
	CrystalGroggyAbilityClass = UGP_CrystalSeraphGroggyAbility::StaticClass();
	CrystalTeleportAbilityClass = UGP_CrystalSeraphTeleportAbility::StaticClass();

	// Crystal Seraph keeps the shared boss patrol/chase/reposition flow and specializes only its scored attack patterns.
	static ConstructorHelpers::FObjectFinder<UBehaviorTree> CrystalBehaviorTreeFinder(TEXT("/Game/Characters/EnemyCharacter/BT/Boss/BT_BossCommon.BT_BossCommon"));
	if (CrystalBehaviorTreeFinder.Succeeded())
	{
		BehaviorTreeAssetOverride = CrystalBehaviorTreeFinder.Object;
	}

	// The boss Blackboard extends the common schema with Crystal Seraph prism, laser, teleport, and core state keys.
	static ConstructorHelpers::FObjectFinder<UBlackboardData> CrystalBlackboardFinder(TEXT("/Game/Characters/EnemyCharacter/BT/Boss/BB_BossCommon.BB_BossCommon"));
	if (CrystalBlackboardFinder.Succeeded())
	{
		BlackboardAssetOverride = CrystalBlackboardFinder.Object;
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->DefaultLandMovementMode = MOVE_Flying;
		MovementComponent->DefaultWaterMovementMode = MOVE_Flying;
		MovementComponent->GravityScale = 0.0f;
		MovementComponent->MaxFlySpeed = 500.0f;
	}

	// SK_MaskMan is the requested prototype mesh for the native Crystal Seraph boss.
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MaskManMeshFinder(TEXT("/Game/Characters/MaskMan/SK_MaskMan.SK_MaskMan"));
	if (MaskManMeshFinder.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(MaskManMeshFinder.Object);
	}
}

void AGP_CrystalSeraphBossCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Reapply shared boss AI assets after serialized Blueprint or placed-instance defaults load.
	if (UBehaviorTree* CrystalBehaviorTree = LoadObject<UBehaviorTree>(nullptr, TEXT("/Game/Characters/EnemyCharacter/BT/Boss/BT_BossCommon.BT_BossCommon")))
	{
		BehaviorTreeAssetOverride = CrystalBehaviorTree;
	}

	// BB_BossCommon supplies the optional Crystal Seraph keys while inheriting the common enemy schema.
	if (UBlackboardData* CrystalBlackboard = LoadObject<UBlackboardData>(nullptr, TEXT("/Game/Characters/EnemyCharacter/BT/Boss/BB_BossCommon.BB_BossCommon")))
	{
		BlackboardAssetOverride = CrystalBlackboard;
	}
}

bool AGP_CrystalSeraphBossCharacter::CanStartCrystalSeraphPattern() const
{
	const UWorld* World = GetWorld();
	return World == nullptr
		|| World->GetTimeSeconds() - LastPatternStartTime >= FMath::Max(0.0f, MinimumPatternInterval);
}

bool AGP_CrystalSeraphBossCharacter::TryStartCrystalSeraphPattern()
{
	if (!HasAuthority() || !CanStartCrystalSeraphPattern())
	{
		return false;
	}

	// Reserve before spawning actors because the BT can immediately execute another task before its service ticks again.
	LastPatternStartTime = GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : LastPatternStartTime;
	return true;
}

void AGP_CrystalSeraphBossCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(CrystalSeraphStateComponent))
	{
		CrystalSeraphStateComponent->OnGroggyChanged.AddUniqueDynamic(this, &ThisClass::HandleCrystalSeraphGroggyChanged);
		CrystalSeraphStateComponent->OnWingCoreExposedChanged.AddUniqueDynamic(this, &ThisClass::HandleCrystalSeraphWingCoreExposedChanged);
	}

	if (!HasAuthority())
	{
		return;
	}

	if (IsValid(CrystalSeraphStateComponent))
	{
		CrystalSeraphStateComponent->InitializeCrystalSeraphState(this);
	}

	GrantCrystalSeraphPatternAbilities();
	MoveToHoverLocation();
}

void AGP_CrystalSeraphBossCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GroggyRecoveryTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

AActor* AGP_CrystalSeraphBossCharacter::RequestSpawnCrystalPrism(AActor* PatternTargetActor)
{
	if (!HasAuthority() || !IsValid(CrystalSeraphStateComponent) || CrystalSeraphStateComponent->IsGroggy())
	{
		return IsValid(CrystalSeraphStateComponent) ? CrystalSeraphStateComponent->GetCrystalPrismActor() : nullptr;
	}

	if (AActor* ExistingPrism = CrystalSeraphStateComponent->GetCrystalPrismActor())
	{
		if (IsValid(ExistingPrism) && !ExistingPrism->IsActorBeingDestroyed())
		{
			return ExistingPrism;
		}
	}

	AActor* TargetActor = ResolvePatternTarget(PatternTargetActor);
	const FVector SpawnLocation = ResolvePrismSpawnLocation(TargetActor);
	const FRotator SpawnRotation = FRotator::ZeroRotator;
	AActor* PrismActor = SpawnConfiguredActor(CrystalPrismActorClass, SpawnLocation, SpawnRotation, TEXT("CrystalPrism"));
	if (AGP_CrystalPrismActor* CrystalPrismActor = Cast<AGP_CrystalPrismActor>(PrismActor))
	{
		CrystalPrismActor->InitializePrism(this);
	}
	CrystalSeraphStateComponent->RegisterCrystalPrismActor(PrismActor);
	LastPrismPatternTime = GetWorld() ? GetWorld()->GetTimeSeconds() : LastPrismPatternTime;
	return PrismActor;
}

bool AGP_CrystalSeraphBossCharacter::RequestStartLaserPattern(AActor* PatternTargetActor)
{
	if (!HasAuthority() || !IsValid(CrystalSeraphStateComponent) || CrystalSeraphStateComponent->IsGroggy())
	{
		return false;
	}

	AActor* TargetActor = ResolvePatternTarget(PatternTargetActor);
	FVector AimDirection = IsValid(TargetActor)
		? (TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal()
		: GetActorForwardVector();
	if (AimDirection.IsNearlyZero())
	{
		AimDirection = GetActorForwardVector().IsNearlyZero() ? FVector::ForwardVector : GetActorForwardVector();
	}

	const FVector SpawnLocation = GetActorLocation() + AimDirection * 120.0f;
	AActor* LaserActor = SpawnConfiguredActor(SeraphLaserActorClass, SpawnLocation, AimDirection.Rotation(), TEXT("SeraphLaser"));
	if (AGP_SeraphLaserActor* SeraphLaserActor = Cast<AGP_SeraphLaserActor>(LaserActor))
	{
		SeraphLaserActor->InitializeLaser(this, TargetActor, AimDirection, 1);
	}
	LastLaserPatternTime = GetWorld() ? GetWorld()->GetTimeSeconds() : LastLaserPatternTime;
	return IsValid(LaserActor);
}

bool AGP_CrystalSeraphBossCharacter::RequestSpawnCrystalShardPattern(AActor* PatternTargetActor)
{
	if (!HasAuthority() || !IsValid(CrystalSeraphStateComponent) || CrystalSeraphStateComponent->IsGroggy() || !*CrystalShardProjectileClass)
	{
		return false;
	}

	AActor* TargetActor = ResolvePatternTarget(PatternTargetActor);
	const FVector TargetLocation = IsValid(TargetActor) ? TargetActor->GetActorLocation() : GetActorLocation() + GetActorForwardVector() * PreferredAirRange;
	FVector Forward = (TargetLocation - GetActorLocation()).GetSafeNormal();
	if (Forward.IsNearlyZero())
	{
		Forward = GetActorForwardVector().IsNearlyZero() ? FVector::ForwardVector : GetActorForwardVector();
	}

	constexpr int32 ShardCount = 5;
	constexpr float FanAngle = 35.0f;
	for (int32 Index = 0; Index < ShardCount; ++Index)
	{
		const float Alpha = ShardCount > 1 ? static_cast<float>(Index) / static_cast<float>(ShardCount - 1) : 0.5f;
		const float YawOffset = FMath::Lerp(-FanAngle * 0.5f, FanAngle * 0.5f, Alpha);
		const FRotator ShardRotation = FRotator(0.0f, YawOffset, 0.0f).RotateVector(Forward).Rotation();
		SpawnConfiguredActor(CrystalShardProjectileClass, GetActorLocation() + Forward * 140.0f, ShardRotation, TEXT("CrystalShard"));
	}

	return *CrystalShardProjectileClass != nullptr;
}

bool AGP_CrystalSeraphBossCharacter::RequestStartAreaPattern(AActor* PatternTargetActor)
{
	if (!HasAuthority() || !IsValid(CrystalSeraphStateComponent) || CrystalSeraphStateComponent->IsGroggy() || !*AreaMarkerActorClass)
	{
		return false;
	}

	AActor* TargetActor = ResolvePatternTarget(PatternTargetActor);
	const FVector BaseLocation = IsValid(TargetActor) ? TargetActor->GetActorLocation() : GetActorLocation();
	const UGP_AttributeSet* GPAttributeSet = Cast<UGP_AttributeSet>(GetAttributeSet());
	const float HealthRatio = IsValid(GPAttributeSet) ? GPAttributeSet->GetHealth() / ResolveBossMaxHealth() : 1.0f;
	const int32 MarkerCount = HealthRatio <= PhaseTwoHealthRatio ? 8 : 5;
	for (int32 Index = 0; Index < MarkerCount; ++Index)
	{
		const float Angle = 360.0f * static_cast<float>(Index) / static_cast<float>(MarkerCount);
		const FVector Offset = FRotator(0.0f, Angle, 0.0f).RotateVector(FVector(320.0f, 0.0f, 0.0f));
		AActor* MarkerActor = SpawnConfiguredActor(AreaMarkerActorClass, BaseLocation + Offset, FRotator::ZeroRotator, TEXT("CrystalAreaMarker"));
		if (AGP_CrystalSanctuaryMarkerActor* SanctuaryMarkerActor = Cast<AGP_CrystalSanctuaryMarkerActor>(MarkerActor))
		{
			SanctuaryMarkerActor->InitializeSanctuaryMarker(this);
		}
	}

	return true;
}

void AGP_CrystalSeraphBossCharacter::RequestExposeWingCore(float ExposureDurationOverride)
{
	if (!HasAuthority() || !IsValid(CrystalSeraphStateComponent))
	{
		return;
	}

	const UGP_AttributeSet* GPAttributeSet = Cast<UGP_AttributeSet>(GetAttributeSet());
	const float BossMaxHealth = ResolveBossMaxHealth();
	const float HealthRatio = IsValid(GPAttributeSet) ? GPAttributeSet->GetHealth() / BossMaxHealth : 1.0f;
	const float EffectiveExposureDuration = ExposureDurationOverride >= 0.0f
		? ExposureDurationOverride
		: (HealthRatio <= PhaseTwoHealthRatio ? PhaseTwoWingCoreExposureDuration : WingCoreExposureDuration);

	CrystalSeraphStateComponent->BeginWingCoreExposure(BossMaxHealth, EffectiveExposureDuration);

	if (!IsValid(CrystalSeraphStateComponent->GetWingCoreActor()))
	{
		const FVector CoreLocation = GetActorLocation() + FVector(0.0f, -90.0f, 120.0f);
		AActor* CoreActor = SpawnConfiguredActor(WingCoreHitActorClass, CoreLocation, GetActorRotation(), TEXT("WingCore"));
		if (IsValid(CoreActor))
		{
			CoreActor->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
			if (AGP_WingCoreHitActor* WingCoreHitActor = Cast<AGP_WingCoreHitActor>(CoreActor))
			{
				WingCoreHitActor->InitializeWingCore(this, CrystalSeraphStateComponent);
				WingCoreHitActor->SetCoreActive(true);
			}
			CrystalSeraphStateComponent->RegisterWingCoreActor(CoreActor);
		}
	}
}

void AGP_CrystalSeraphBossCharacter::RequestWingCoreBreak()
{
	if (!HasAuthority() || !IsValid(CrystalSeraphStateComponent))
	{
		return;
	}

	CrystalSeraphStateComponent->SetWingCoreBreakCount(CrystalSeraphStateComponent->GetWingCoreBreakCount() + 1);
	if (CrystalSeraphStateComponent->GetWingCoreBreakCount() >= CrystalSeraphStateComponent->GetWingCoreBreakTarget())
	{
		RequestEnterGroggy();
	}
	else
	{
		CrystalSeraphStateComponent->EndWingCoreExposure();
	}
}

void AGP_CrystalSeraphBossCharacter::RequestEnterGroggy()
{
	if (!HasAuthority() || !IsValid(CrystalSeraphStateComponent))
	{
		return;
	}

	CrystalSeraphStateComponent->EnterGroggy();
}

void AGP_CrystalSeraphBossCharacter::RequestRecoverFromGroggy()
{
	if (!HasAuthority() || !IsValid(CrystalSeraphStateComponent))
	{
		return;
	}

	CrystalSeraphStateComponent->RecoverFromGroggy();
	MoveToHoverLocation();
}

bool AGP_CrystalSeraphBossCharacter::RequestTeleportToPreferredCombatPosition(AActor* PatternTargetActor)
{
	if (!HasAuthority() || !IsValid(CrystalSeraphStateComponent) || CrystalSeraphStateComponent->IsGroggy())
	{
		return false;
	}

	AActor* TargetActor = ResolvePatternTarget(PatternTargetActor);
	if (!IsValid(TargetActor))
	{
		return false;
	}

	const float WorldTimeSeconds = GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.0f;
	if (WorldTimeSeconds - LastTacticalTeleportTime < FMath::Max(0.0f, TacticalTeleportCooldown))
	{
		// Repeated BT evaluations during the cooldown are already satisfied by the previous teleport.
		return true;
	}

	FVector HorizontalAway = (GetActorLocation() - TargetActor->GetActorLocation()).GetSafeNormal2D();
	if (HorizontalAway.IsNearlyZero())
	{
		HorizontalAway = -TargetActor->GetActorForwardVector().GetSafeNormal2D();
	}
	if (HorizontalAway.IsNearlyZero())
	{
		HorizontalAway = FVector::ForwardVector;
	}
	const float OrbitAngleDegrees = TacticalTeleportSequence % 2 == 0 ? 45.0f : -45.0f;
	HorizontalAway = HorizontalAway.RotateAngleAxis(OrbitAngleDegrees, FVector::UpVector).GetSafeNormal2D();
	++TacticalTeleportSequence;

	const FVector TargetLocation = TargetActor->GetActorLocation();
	const FVector Destination(
		TargetLocation.X + HorizontalAway.X * GetPreferredAirRange(),
		TargetLocation.Y + HorizontalAway.Y * GetPreferredAirRange(),
		TargetLocation.Z + GetPreferredHoverHeight());
	const FRotator FacingRotation = (TargetLocation - Destination).Rotation();

	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->StopMovement();
	}

	SetActorLocationAndRotation(Destination, FRotator(0.0f, FacingRotation.Yaw, 0.0f), false, nullptr, ETeleportType::TeleportPhysics);
	LastTacticalTeleportTime = WorldTimeSeconds;

	UE_LOG(LogTemp, Log, TEXT("[CrystalSeraph] Tactical teleport: Boss=%s Target=%s Destination=%s"),
		*GetNameSafe(this),
		*GetNameSafe(TargetActor),
		*Destination.ToCompactString());
	return true;
}

bool AGP_CrystalSeraphBossCharacter::ShouldTeleportForCrystalSeraph(float DistanceToTarget) const
{
	const float SafeDistance = FMath::Max(0.0f, DistanceToTarget);
	return SafeDistance <= CloseTeleportDistance || SafeDistance >= FarTeleportDistance;
}

void AGP_CrystalSeraphBossCharacter::HandlePostDamageTaken(AActor* InstigatorActor, float DamageAmount, FGameplayTag ElementTag)
{
	Super::HandlePostDamageTaken(InstigatorActor, DamageAmount, ElementTag);

	if (!HasAuthority() || !IsValid(CrystalSeraphStateComponent) || !CrystalSeraphStateComponent->IsWingCoreExposed())
	{
		return;
	}

	const bool bCoreBroken = CrystalSeraphStateComponent->RecordWingCoreDamage(DamageAmount, ResolveBossMaxHealth());
	if (bCoreBroken)
	{
		UE_LOG(LogTemp, Log, TEXT("[CrystalSeraph] Wing core broke. Boss=%s BreakCount=%d/%d"),
			*GetNameSafe(this),
			CrystalSeraphStateComponent->GetWingCoreBreakCount(),
			CrystalSeraphStateComponent->GetWingCoreBreakTarget());
	}
}

void AGP_CrystalSeraphBossCharacter::HandleCrystalSeraphGroggyChanged(bool bNewGroggy)
{
	if (!HasAuthority())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GroggyRecoveryTimerHandle);
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->GravityScale = bNewGroggy ? 1.0f : 0.0f;
		MovementComponent->SetMovementMode(bNewGroggy ? MOVE_Walking : MOVE_Flying);
	}

	if (bNewGroggy)
	{
		SetActorLocation(ResolveGroundGroggyLocation(), false, nullptr, ETeleportType::TeleportPhysics);
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				GroggyRecoveryTimerHandle,
				this,
				&ThisClass::RequestRecoverFromGroggy,
				ResolveCurrentGroggyDuration(),
				false);
		}
	}
}

void AGP_CrystalSeraphBossCharacter::HandleCrystalSeraphWingCoreExposedChanged(bool bNewExposed)
{
	if (!HasAuthority())
	{
		return;
	}

	if (AActor* WingCoreActor = IsValid(CrystalSeraphStateComponent) ? CrystalSeraphStateComponent->GetWingCoreActor() : nullptr)
	{
		if (AGP_WingCoreHitActor* WingCoreHitActor = Cast<AGP_WingCoreHitActor>(WingCoreActor))
		{
			WingCoreHitActor->SetCoreActive(bNewExposed);
			return;
		}

		WingCoreActor->SetActorHiddenInGame(!bNewExposed);
		WingCoreActor->SetActorEnableCollision(bNewExposed);
	}
}

void AGP_CrystalSeraphBossCharacter::GrantCrystalSeraphPatternAbilities()
{
	if (!HasAuthority() || !bGrantCrystalSeraphPatternAbilities)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!IsValid(ASC))
	{
		return;
	}

	const TSubclassOf<UGameplayAbility> CrystalAbilities[] =
	{
		CrystalShardAbilityClass,
		CrystalLaserAbilityClass,
		CrystalPrismAbilityClass,
		CrystalAreaAbilityClass,
		CrystalGroggyAbilityClass,
		CrystalTeleportAbilityClass,
	};

	for (const TSubclassOf<UGameplayAbility>& AbilityClass : CrystalAbilities)
	{
		if (!*AbilityClass || ASC->FindAbilitySpecFromClass(AbilityClass) != nullptr)
		{
			continue;
		}

		// Native grants keep the boss playable before a Blueprint child supplies custom pattern abilities.
		ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass));
	}
}

AActor* AGP_CrystalSeraphBossCharacter::ResolvePatternTarget(AActor* ExplicitTargetActor) const
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

	// Keep the native prototype usable in PIE before a dedicated Crystal Seraph BT asset exists.
	return UGameplayStatics::GetPlayerPawn(this, 0);
}

FVector AGP_CrystalSeraphBossCharacter::ResolvePrismSpawnLocation(AActor* TargetActor) const
{
	const FVector TargetLocation = IsValid(TargetActor) ? TargetActor->GetActorLocation() : GetActorLocation() + GetActorForwardVector() * PreferredAirRange;
	FVector Right = GetActorRightVector().GetSafeNormal2D();
	if (Right.IsNearlyZero())
	{
		Right = FVector::RightVector;
	}

	const float SideSign = FMath::RandBool() ? 1.0f : -1.0f;
	FVector DesiredLocation = TargetLocation + Right * SideSign * FMath::RandRange(500.0f, 800.0f);

	if (UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld()))
	{
		FNavLocation ProjectedLocation;
		// Prism placement stays on navigable combat space so the player can read and reach the mechanic.
		if (NavigationSystem->ProjectPointToNavigation(DesiredLocation, ProjectedLocation, FVector(280.0f, 280.0f, 360.0f)))
		{
			DesiredLocation = ProjectedLocation.Location;
		}
	}

	if (IsValid(TargetActor) && FVector::Dist2D(DesiredLocation, TargetActor->GetActorLocation()) < 400.0f)
	{
		DesiredLocation += (DesiredLocation - TargetActor->GetActorLocation()).GetSafeNormal2D() * 400.0f;
	}

	return DesiredLocation;
}

FVector AGP_CrystalSeraphBossCharacter::ResolveGroundGroggyLocation() const
{
	const FVector TraceStart = GetActorLocation();
	const FVector TraceEnd = TraceStart - FVector(0.0f, 0.0f, 5000.0f);
	FHitResult GroundHit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CrystalSeraphGroggyGround), false, this);
	if (GetWorld() && GetWorld()->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
	{
		return GroundHit.ImpactPoint + FVector(0.0f, 0.0f, 96.0f);
	}

	return GetActorLocation() - FVector(0.0f, 0.0f, PreferredHoverHeight);
}

FVector AGP_CrystalSeraphBossCharacter::ResolveHoverLocation() const
{
	AActor* TargetActor = ResolvePatternTarget(nullptr);
	const FVector BaseLocation = IsValid(TargetActor) ? TargetActor->GetActorLocation() : GetActorLocation();
	return FVector(GetActorLocation().X, GetActorLocation().Y, BaseLocation.Z + PreferredHoverHeight);
}

float AGP_CrystalSeraphBossCharacter::ResolveBossMaxHealth() const
{
	const UGP_AttributeSet* GPAttributeSet = Cast<UGP_AttributeSet>(GetAttributeSet());
	return IsValid(GPAttributeSet) ? FMath::Max(1.0f, GPAttributeSet->GetMaxHealth()) : 1000.0f;
}

float AGP_CrystalSeraphBossCharacter::ResolveCurrentGroggyDuration() const
{
	const UGP_AttributeSet* GPAttributeSet = Cast<UGP_AttributeSet>(GetAttributeSet());
	const float MaxHealth = ResolveBossMaxHealth();
	const float HealthRatio = IsValid(GPAttributeSet) ? GPAttributeSet->GetHealth() / MaxHealth : 1.0f;
	return HealthRatio <= FinalPhaseHealthRatio ? FinalPhaseGroggyDuration : GroggyDuration;
}

void AGP_CrystalSeraphBossCharacter::MoveToHoverLocation()
{
	if (!HasAuthority())
	{
		return;
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->GravityScale = 0.0f;
		MovementComponent->SetMovementMode(MOVE_Flying);
	}

	SetActorLocation(ResolveHoverLocation(), false, nullptr, ETeleportType::TeleportPhysics);
}

AActor* AGP_CrystalSeraphBossCharacter::SpawnConfiguredActor(TSubclassOf<AActor> ActorClass, const FVector& Location, const FRotator& Rotation, FName DebugName)
{
	if (!HasAuthority() || !*ActorClass || !GetWorld())
	{
		UE_LOG(LogTemp, Verbose, TEXT("[CrystalSeraph] Missing actor class for %s on %s"), *DebugName.ToString(), *GetNameSafe(this));
		return nullptr;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	return GetWorld()->SpawnActor<AActor>(ActorClass, Location, Rotation, SpawnParameters);
}
