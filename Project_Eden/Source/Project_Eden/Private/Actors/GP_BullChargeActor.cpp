#include "Actors/GP_BullChargeActor.h"

#include "Animation/AnimationAsset.h"
#include "Actors/GP_MatadorBossDecoyActor.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Characters/GP_MatadorBossStateComponent.h"
#include "Components/BoxComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "GameplayTags/GP_Tags.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Utils/GP_BlueprintLibrary.h"

AGP_BullChargeActor::AGP_BullChargeActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetBoxExtent(FVector(120.0f, 65.0f, 75.0f));
	CollisionBox->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionBox->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	SetRootComponent(CollisionBox);

	BullVisualMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BullVisualMesh"));
	BullVisualMesh->SetupAttachment(CollisionBox);
	BullVisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BullVisualMesh->SetGenerateOverlapEvents(false);
	BullVisualMesh->SetHiddenInGame(true);
	BullVisualMesh->SetVisibility(false);

	BullActorVisual = CreateDefaultSubobject<UChildActorComponent>(TEXT("BullActorVisual"));
	BullActorVisual->SetupAttachment(CollisionBox);
	BullActorVisual->SetRelativeLocation(FVector(0.0f, 0.0f, -75.0f));
	BullActorVisual->SetRelativeScale3D(FVector(0.65f));

	static ConstructorHelpers::FClassFinder<AActor> BullBlueprintFinder(TEXT("/Game/Meshes/Bull/SK/Bull"));
	if (BullBlueprintFinder.Succeeded())
	{
		BullActorVisual->SetChildActorClass(BullBlueprintFinder.Class);
	}
	else
	{
		BullVisualMesh->SetHiddenInGame(false);
		BullVisualMesh->SetVisibility(true);

		static ConstructorHelpers::FObjectFinder<USkeletalMesh> BullMeshFinder(TEXT("/Game/Meshes/Bull/SK/Bull_High.Bull_High"));
		if (BullMeshFinder.Succeeded())
		{
			BullVisualMesh->SetSkeletalMesh(BullMeshFinder.Object);
			BullVisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -75.0f));
			BullVisualMesh->SetRelativeScale3D(FVector(0.65f));
		}

		static ConstructorHelpers::FObjectFinder<UAnimationAsset> BullRunFinder(TEXT("/Game/Meshes/Bull/SK/Bull_run_ScaleFix100.Bull_run_ScaleFix100"));
		if (BullRunFinder.Succeeded())
		{
			BullVisualMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
			BullVisualMesh->SetAnimation(BullRunFinder.Object);
			BullVisualMesh->SetPlayRate(1.25f);
			BullVisualMesh->Play(true);
		}

		static ConstructorHelpers::FObjectFinder<UMaterialInterface> BullMaterialFinder(TEXT("/Game/Fab/Lava_Material/Material/MI_Bull_HeatedMagma.MI_Bull_HeatedMagma"));
		if (BullMaterialFinder.Succeeded())
		{
			BullVisualMesh->SetMaterial(0, BullMaterialFinder.Object);
		}
	}

	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageEffectFinder(TEXT("/Game/GAS_Pattern/AbilitySystem/GameplayEffects/Damage/GE_PrimaryDamage"));
	if (DamageEffectFinder.Succeeded())
	{
		DamageEffectClass = DamageEffectFinder.Class;
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> BullSpawnEffectFinder(TEXT("/Game/Imported_VFX/Free_Magic/VFX_Niagara/NS_Free_Magic_Area2.NS_Free_Magic_Area2"));
	if (BullSpawnEffectFinder.Succeeded())
	{
		BullSpawnEffect = BullSpawnEffectFinder.Object;
	}

	PlayerHitEventTag = GPTags::Event::Player::HitReact;
}

void AGP_BullChargeActor::BeginPlay()
{
	Super::BeginPlay();

	SetLifeSpan(MaxChargeLifeSeconds + TelegraphLeadTime + 0.5f);
	CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnChargeOverlap);
	BP_OnBullSpawnPresentation();
	if (BullSpawnEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			BullSpawnEffect,
			GetActorLocation() + BullSpawnEffectOffset,
			GetActorRotation(),
			BullSpawnEffectScale,
			true,
			true,
			ENCPoolMethod::AutoRelease);
	}
	DrawTelegraph();
}

void AGP_BullChargeActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGP_BullChargeActor, ChargeState);
}

void AGP_BullChargeActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority() || !bChargeStarted || bChargeFinished)
	{
		return;
	}

	if (ChargeState == EGPMatadorBullChargeState::CirclingDecoyToPlayer)
	{
		TickDecoyCircleRedirect(DeltaSeconds);
		return;
	}

	if (TryHandleDecoyProximity())
	{
		return;
	}

	const FVector DesiredDirection = ResolveDesiredDirection();
	const float MaxTurnRadians = FMath::DegreesToRadians(HomingTurnSpeedDegrees) * DeltaSeconds;
	ChargeDirection = FMath::VInterpNormalRotationTo(ChargeDirection, DesiredDirection, DeltaSeconds, FMath::Max(0.0f, HomingTurnSpeedDegrees / 90.0f));
	if (FMath::Acos(FMath::Clamp(FVector::DotProduct(ChargeDirection, DesiredDirection), -1.0f, 1.0f)) <= MaxTurnRadians)
	{
		ChargeDirection = DesiredDirection;
	}

	const FVector PreviousLocation = GetActorLocation();
	FHitResult MoveHit;
	AddActorWorldOffset(ChargeDirection * ChargeSpeed * DeltaSeconds, true, &MoveHit);
	if (MoveHit.bBlockingHit)
	{
		FinishCharge(false);
		return;
	}

	TryHandleDecoyProximity(&PreviousLocation);
}

void AGP_BullChargeActor::InitializeBullCharge(AActor* InInitialTargetActor, AActor* InPlayerTargetActor, AGP_MatadorBossDecoyActor* InDecoyActor, UGP_MatadorBossStateComponent* InStateComponent, const FVector& InChargeDirection)
{
	TargetActor = InInitialTargetActor;
	PlayerTargetActor = InPlayerTargetActor;
	DecoyActor = InDecoyActor;
	MatadorStateComponent = InStateComponent;
	SetChargeState(EGPMatadorBullChargeState::SpawnTelegraph);

	const FVector SafeDirection = InChargeDirection.GetSafeNormal2D();
	ChargeDirection = SafeDirection.IsNearlyZero() ? GetActorForwardVector().GetSafeNormal2D() : SafeDirection;
	if (ChargeDirection.IsNearlyZero())
	{
		ChargeDirection = FVector::ForwardVector;
	}

	SetActorRotation(ChargeDirection.Rotation());
	DrawTelegraph();

	if (HasAuthority())
	{
		if (IsValid(MatadorStateComponent.Get()))
		{
			MatadorStateComponent->RegisterActiveBullActor(this);
		}

		GetWorldTimerManager().SetTimer(ChargeStartTimerHandle, this, &ThisClass::StartCharge, FMath::Max(0.0f, TelegraphLeadTime), false);
	}
}

void AGP_BullChargeActor::RedirectTowardActor(AActor* NewTargetActor)
{
	if (!HasAuthority() || !IsValid(NewTargetActor))
	{
		return;
	}

	// Player counterplay can call this from an overlap, notify, or Blueprint damage reaction.
	TargetActor = NewTargetActor;
	SetChargeState(NewTargetActor == DecoyActor.Get()
		? EGPMatadorBullChargeState::RedirectedByPlayerToDecoy
		: EGPMatadorBullChargeState::ChargingToPlayer);
	BP_OnBullRedirected(nullptr, NewTargetActor);
}

bool AGP_BullChargeActor::TryRedirectTowardDecoy(AActor* RedirectingActor)
{
	if (!HasAuthority() || !CanRedirectTowardDecoy(RedirectingActor))
	{
		return false;
	}

	bRedirectedByPlayer = true;
	TargetActor = DecoyActor.Get();
	SetChargeState(EGPMatadorBullChargeState::RedirectedByPlayerToDecoy);
	BP_OnBullRedirected(RedirectingActor, DecoyActor.Get());

	UE_LOG(LogTemp, Log, TEXT("[MatadorBull] Redirected toward decoy. Bull=%s Redirector=%s Decoy=%s"),
		*GetNameSafe(this),
		*GetNameSafe(RedirectingActor),
		*GetNameSafe(DecoyActor.Get()));

	return true;
}

bool AGP_BullChargeActor::CanRedirectTowardDecoy(AActor* RedirectingActor) const
{
	if (!IsValid(RedirectingActor) || !IsValid(DecoyActor.Get()) || bChargeFinished || bRedirectedByPlayer)
	{
		return false;
	}

	if (!bChargeStarted || ChargeState != EGPMatadorBullChargeState::RedirectedByDecoyToPlayer)
	{
		return false;
	}

	const FVector BullLocation = GetActorLocation();
	const FVector RedirectorLocation = RedirectingActor->GetActorLocation();
	if (FVector::DistSquared2D(BullLocation, RedirectorLocation) > FMath::Square(FMath::Max(0.0f, RedirectMaxDistance)))
	{
		return false;
	}

	const FVector ToRedirector = (RedirectorLocation - BullLocation).GetSafeNormal2D();
	const FVector CurrentDirection = ChargeDirection.GetSafeNormal2D();
	if (ToRedirector.IsNearlyZero() || CurrentDirection.IsNearlyZero())
	{
		return false;
	}

	const float Dot = FVector::DotProduct(CurrentDirection, ToRedirector);
	const float MinDot = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(RedirectMaxAngleDegrees, 0.0f, 180.0f)));
	return Dot >= MinDot;
}

void AGP_BullChargeActor::StartCharge()
{
	if (bChargeFinished)
	{
		return;
	}

	bChargeStarted = true;
	SetChargeState(EGPMatadorBullChargeState::ChargingToDecoy);
	BP_OnBullChargeStarted();
}

void AGP_BullChargeActor::OnChargeOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !bChargeStarted || bChargeFinished || !IsValid(OtherActor) || OtherActor == this || OtherActor == GetOwner())
	{
		return;
	}

	if (OtherActor == DecoyActor.Get())
	{
		if (ChargeState == EGPMatadorBullChargeState::ChargingToDecoy)
		{
			if (StartDecoyCircleRedirect())
			{
				return;
			}

			SetChargeState(EGPMatadorBullChargeState::Failed);
			FinishCharge(false);
			return;
		}

		if (!CanRecordDecoyHit())
		{
			UE_LOG(LogTemp, Log, TEXT("[MatadorBull] Decoy hit rejected. Bull=%s State=%d Decoy=%s"),
				*GetNameSafe(this),
				static_cast<int32>(ChargeState),
				*GetNameSafe(DecoyActor.Get()));
			SetChargeState(EGPMatadorBullChargeState::Failed);
			FinishCharge(false);
			return;
		}

		if (!BeginChargeImpact())
		{
			return;
		}

		SetChargeState(EGPMatadorBullChargeState::HitDecoy);
		if (IsValid(MatadorStateComponent.Get()))
		{
			MatadorStateComponent->RecordBullHitDecoy();
		}
		if (IsValid(MatadorStateComponent.Get()) && MatadorStateComponent->IsGroggy())
		{
			DecoyActor->PlayBreakPresentation();
		}
		else if (IsValid(MatadorStateComponent.Get()))
		{
			DecoyActor->PlayBullReturnPresentation(MatadorStateComponent->GetChainBreakCount(), MatadorStateComponent->GetChainBreakTarget());
		}
		FinishCharge(true);
		return;
	}

	if (UGP_BlueprintLibrary::CanApplyCombatEffect(GetOwner(), OtherActor))
	{
		if (bRedirectTowardDecoyOnPlayerOverlap && TryRedirectTowardDecoy(OtherActor))
		{
			return;
		}

		if (!BeginChargeImpact())
		{
			return;
		}

		SetChargeState(EGPMatadorBullChargeState::Failed);
		AActor* InstigatorActor = GetOwner();
		UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InstigatorActor);
		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
		if (IsValid(SourceASC) && IsValid(TargetASC) && DamageEffectClass)
		{
			FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
			ContextHandle.AddInstigator(InstigatorActor, InstigatorActor);

			FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, EffectLevel, ContextHandle);
			if (SpecHandle.IsValid())
			{
				SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Data::Base, BullHitBaseDamage);
				SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Data::BaseSpell, 0.0f);
				SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Data::ToughnessBase, BullHitToughnessDamage);
				SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Coef::Atk, BullHitAttackPowerCoefficient);
				SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Coef::M_Atk, 0.0f);
				SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Coef::Def, 0.0f);
				SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Coef::Hp, 0.0f);
				SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
			}
		}

		TArray<AActor*> HitActors;
		HitActors.Add(OtherActor);
		UGP_BlueprintLibrary::SendGameplayEventToActors(InstigatorActor, HitActors, PlayerHitEventTag);
		FinishCharge(false);
	}
}

void AGP_BullChargeActor::SetChargeState(EGPMatadorBullChargeState NewState)
{
	if (ChargeState == NewState)
	{
		return;
	}

	ChargeState = NewState;
	UE_LOG(LogTemp, Verbose, TEXT("[MatadorBull] State changed. Bull=%s State=%d"),
		*GetNameSafe(this),
		static_cast<int32>(ChargeState));
}

bool AGP_BullChargeActor::TryHandleDecoyProximity(const FVector* PreviousLocation)
{
	if (!HasAuthority() || !bChargeStarted || bChargeFinished || !IsValid(DecoyActor.Get()))
	{
		return false;
	}

	const FVector DecoyLocation = DecoyActor->GetActorLocation();
	float DistanceSquaredToDecoy = FVector::DistSquared2D(GetActorLocation(), DecoyLocation);
	if (PreviousLocation)
	{
		const FVector SegmentStart(PreviousLocation->X, PreviousLocation->Y, DecoyLocation.Z);
		const FVector SegmentEnd(GetActorLocation().X, GetActorLocation().Y, DecoyLocation.Z);
		const FVector ClosestPoint = FMath::ClosestPointOnSegment(DecoyLocation, SegmentStart, SegmentEnd);
		DistanceSquaredToDecoy = FMath::Min(DistanceSquaredToDecoy, FVector::DistSquared2D(ClosestPoint, DecoyLocation));
	}

	if (ChargeState == EGPMatadorBullChargeState::ChargingToDecoy
		&& DistanceSquaredToDecoy <= FMath::Square(FMath::Max(0.0f, DecoyRedirectRadius)))
	{
		UE_LOG(LogTemp, Log, TEXT("[MatadorBull] Decoy proximity starts circle redirect. Bull=%s Decoy=%s Distance=%.1f Radius=%.1f"),
			*GetNameSafe(this),
			*GetNameSafe(DecoyActor.Get()),
			FMath::Sqrt(DistanceSquaredToDecoy),
			DecoyRedirectRadius);

		if (StartDecoyCircleRedirect())
		{
			return true;
		}

		SetChargeState(EGPMatadorBullChargeState::Failed);
		FinishCharge(false);
		return true;
	}

	if (ChargeState == EGPMatadorBullChargeState::RedirectedByPlayerToDecoy
		&& DistanceSquaredToDecoy <= FMath::Square(FMath::Max(0.0f, DecoyReturnHitRadius)))
	{
		UE_LOG(LogTemp, Log, TEXT("[MatadorBull] Decoy proximity return hit. Bull=%s Decoy=%s Distance=%.1f Radius=%.1f"),
			*GetNameSafe(this),
			*GetNameSafe(DecoyActor.Get()),
			FMath::Sqrt(DistanceSquaredToDecoy),
			DecoyReturnHitRadius);

		if (!BeginChargeImpact())
		{
			return true;
		}

		SetChargeState(EGPMatadorBullChargeState::HitDecoy);
		if (IsValid(MatadorStateComponent.Get()))
		{
			MatadorStateComponent->RecordBullHitDecoy();
		}
		if (IsValid(MatadorStateComponent.Get()) && MatadorStateComponent->IsGroggy())
		{
			DecoyActor->PlayBreakPresentation();
		}
		else if (IsValid(MatadorStateComponent.Get()))
		{
			DecoyActor->PlayBullReturnPresentation(MatadorStateComponent->GetChainBreakCount(), MatadorStateComponent->GetChainBreakTarget());
		}
		FinishCharge(true);
		return true;
	}

	return false;
}

bool AGP_BullChargeActor::StartDecoyCircleRedirect()
{
	AActor* PlayerTarget = ResolvePlayerTargetActor();
	if (!HasAuthority() || bRedirectedByDecoy || !IsValid(PlayerTarget) || !IsValid(DecoyActor.Get()) || bChargeFinished)
	{
		return false;
	}

	FVector StartOffset = GetActorLocation() - DecoyActor->GetActorLocation();
	StartOffset.Z = 0.0f;
	if (StartOffset.IsNearlyZero())
	{
		StartOffset = -ChargeDirection.GetSafeNormal2D() * FMath::Max(1.0f, DecoyRedirectArcRadius);
	}

	DecoyCircleRadius = FMath::Max(FMath::Max(1.0f, DecoyRedirectArcRadius), StartOffset.Size2D());
	DecoyCircleStartOffset = StartOffset.GetSafeNormal2D() * DecoyCircleRadius;
	DecoyCircleProgressDegrees = 0.0f;

	const FVector ToPlayer = (PlayerTarget->GetActorLocation() - DecoyActor->GetActorLocation()).GetSafeNormal2D();
	const FVector StartDirection = DecoyCircleStartOffset.GetSafeNormal2D();
	const float CrossZ = FVector::CrossProduct(StartDirection, ToPlayer).Z;
	DecoyCircleDirectionSign = CrossZ >= 0.0f ? 1.0f : -1.0f;

	TargetActor = nullptr;
	SetChargeState(EGPMatadorBullChargeState::CirclingDecoyToPlayer);
	DecoyActor->PlayBullRedirectPresentation(this, PlayerTarget);

	UE_LOG(LogTemp, Log, TEXT("[MatadorBull] Decoy circle redirect started. Bull=%s Decoy=%s Player=%s Arc=%.1f Radius=%.1f Sign=%.0f"),
		*GetNameSafe(this),
		*GetNameSafe(DecoyActor.Get()),
		*GetNameSafe(PlayerTarget),
		DecoyRedirectArcDegrees,
		DecoyCircleRadius,
		DecoyCircleDirectionSign);

	return true;
}

void AGP_BullChargeActor::TickDecoyCircleRedirect(float DeltaSeconds)
{
	if (!HasAuthority() || !IsValid(DecoyActor.Get()) || bChargeFinished)
	{
		FinishCharge(false);
		return;
	}

	AActor* PlayerTarget = ResolvePlayerTargetActor();
	if (!IsValid(PlayerTarget))
	{
		FinishCharge(false);
		return;
	}

	const FVector CenterLocation = DecoyActor->GetActorLocation();
	FVector CurrentOffset = GetActorLocation() - CenterLocation;
	CurrentOffset.Z = 0.0f;
	if (CurrentOffset.IsNearlyZero())
	{
		CurrentOffset = DecoyCircleStartOffset;
	}

	const FVector RadialDirection = CurrentOffset.GetSafeNormal2D();
	const FVector TangentDirection = FRotator(0.0f, 90.0f * DecoyCircleDirectionSign, 0.0f).RotateVector(RadialDirection).GetSafeNormal2D();
	const FVector ToPlayerDirection = (PlayerTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	const float CurrentRadius = FMath::Max(1.0f, CurrentOffset.Size2D());
	const float RadiusCorrectionAlpha = FMath::Clamp((DecoyCircleRadius - CurrentRadius) / FMath::Max(1.0f, DecoyCircleRadius), -1.0f, 1.0f);
	const FVector RadiusCorrection = RadialDirection * RadiusCorrectionAlpha;
	FVector DesiredDirection = TangentDirection * 0.70f + ToPlayerDirection * 0.45f + RadiusCorrection * 0.85f;
	DesiredDirection = DesiredDirection.GetSafeNormal2D();
	if (DesiredDirection.IsNearlyZero())
	{
		DesiredDirection = TangentDirection;
	}

	const float SteeringSpeed = FMath::Max(HomingTurnSpeedDegrees, DecoyRedirectArcSpeedDegrees);
	ChargeDirection = FMath::VInterpNormalRotationTo(ChargeDirection, DesiredDirection, DeltaSeconds, FMath::Max(0.0f, SteeringSpeed / 90.0f));
	if (ChargeDirection.IsNearlyZero())
	{
		ChargeDirection = DesiredDirection;
	}

	FHitResult MoveHit;
	AddActorWorldOffset(ChargeDirection * ChargeSpeed * DeltaSeconds, true, &MoveHit);
	if (MoveHit.bBlockingHit)
	{
		FinishCharge(false);
		return;
	}
	SetActorRotation(ChargeDirection.Rotation());

	const FVector CurrentDirectionFromDecoy = (GetActorLocation() - CenterLocation).GetSafeNormal2D();
	const FVector StartDirectionFromDecoy = DecoyCircleStartOffset.GetSafeNormal2D();
	const float Dot = FVector::DotProduct(StartDirectionFromDecoy, CurrentDirectionFromDecoy);
	const float CrossZ = FVector::CrossProduct(StartDirectionFromDecoy, CurrentDirectionFromDecoy).Z;
	float SignedProgress = FMath::RadiansToDegrees(FMath::Atan2(CrossZ, Dot)) * DecoyCircleDirectionSign;
	if (SignedProgress < 0.0f)
	{
		SignedProgress += 360.0f;
	}
	DecoyCircleProgressDegrees = FMath::Max(DecoyCircleProgressDegrees, SignedProgress);

	DrawDebugCircle(
		GetWorld(),
		CenterLocation + FVector(0.0f, 0.0f, 35.0f),
		DecoyCircleRadius,
		48,
		FColor::Cyan,
		false,
		0.05f,
		0,
		4.0f,
		FVector::ForwardVector,
		FVector::RightVector,
		false);

	const float ArcTargetDegrees = FMath::Clamp(DecoyRedirectArcDegrees, 0.0f, 360.0f);
	if (DecoyCircleProgressDegrees >= ArcTargetDegrees - KINDA_SMALL_NUMBER)
	{
		if (!TryRedirectTowardPlayerFromDecoy())
		{
			UE_LOG(LogTemp, Warning, TEXT("[MatadorBull] Decoy circle finished but player redirect failed. Bull=%s Player=%s Decoy=%s"),
				*GetNameSafe(this),
				*GetNameSafe(PlayerTargetActor.Get()),
				*GetNameSafe(DecoyActor.Get()));
			FinishCharge(false);
		}
	}
}

AActor* AGP_BullChargeActor::ResolvePlayerTargetActor()
{
	if (IsValid(PlayerTargetActor.Get()))
	{
		return PlayerTargetActor.Get();
	}

	AActor* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (IsValid(PlayerPawn))
	{
		PlayerTargetActor = PlayerPawn;
		return PlayerPawn;
	}

	return nullptr;
}

bool AGP_BullChargeActor::TryRedirectTowardPlayerFromDecoy()
{
	AActor* PlayerTarget = ResolvePlayerTargetActor();
	if (!HasAuthority() || bRedirectedByDecoy || !IsValid(PlayerTarget) || !IsValid(DecoyActor.Get()) || bChargeFinished)
	{
		return false;
	}

	bRedirectedByDecoy = true;
	TargetActor = PlayerTarget;
	const FVector ToPlayer = (PlayerTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (!ToPlayer.IsNearlyZero())
	{
		ChargeDirection = ToPlayer;
		SetActorRotation(ChargeDirection.Rotation());
	}
	SetChargeState(EGPMatadorBullChargeState::RedirectedByDecoyToPlayer);
	DecoyActor->PlayBullRedirectPresentation(this, PlayerTarget);
	BP_OnBullRedirected(DecoyActor.Get(), PlayerTarget);

	UE_LOG(LogTemp, Log, TEXT("[MatadorBull] Decoy redirected bull toward player. Bull=%s Decoy=%s Player=%s Direction=%s"),
		*GetNameSafe(this),
		*GetNameSafe(DecoyActor.Get()),
		*GetNameSafe(PlayerTarget),
		*ChargeDirection.ToCompactString());

	return true;
}

bool AGP_BullChargeActor::CanRecordDecoyHit() const
{
	return ChargeState == EGPMatadorBullChargeState::RedirectedByPlayerToDecoy;
}

void AGP_BullChargeActor::FinishCharge(bool bHitDecoy)
{
	if (bChargeFinished)
	{
		return;
	}

	bChargeFinished = true;
	bImpactHandled = true;
	if (ChargeState != EGPMatadorBullChargeState::HitDecoy && ChargeState != EGPMatadorBullChargeState::Failed)
	{
		SetChargeState(bHitDecoy ? EGPMatadorBullChargeState::HitDecoy : EGPMatadorBullChargeState::Failed);
	}
	if (IsValid(CollisionBox))
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	BP_OnBullChargeEnded(bHitDecoy);
	SetChargeState(EGPMatadorBullChargeState::Finished);

	if (IsValid(MatadorStateComponent.Get()) && MatadorStateComponent->GetActiveBullActor() == this)
	{
		MatadorStateComponent->RegisterActiveBullActor(nullptr);
	}

	if (HasAuthority())
	{
		Destroy();
	}
}

bool AGP_BullChargeActor::BeginChargeImpact()
{
	if (bImpactHandled || bChargeFinished)
	{
		return false;
	}

	bImpactHandled = true;
	if (IsValid(CollisionBox))
	{
		// Prevent repeated overlap callbacks from applying damage every frame while Destroy is pending.
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	return true;
}

void AGP_BullChargeActor::DrawTelegraph() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Debug telegraph keeps the prototype readable before final VFX assets exist.
	const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 35.0f);
	const FVector End = Start + ChargeDirection.GetSafeNormal() * TelegraphLength;
	const FVector BoxExtent = IsValid(CollisionBox) ? CollisionBox->GetScaledBoxExtent() : FVector(90.0f, 60.0f, 45.0f);
	DrawDebugLine(World, Start, End, FColor::Orange, false, FMath::Max(0.1f, TelegraphLeadTime), 0, 12.0f);
	DrawDebugBox(World, End, BoxExtent, GetActorQuat(), FColor::Orange, false, FMath::Max(0.1f, TelegraphLeadTime), 0, 6.0f);
}

FVector AGP_BullChargeActor::ResolveDesiredDirection() const
{
	if (IsValid(TargetActor))
	{
		const FVector ToTarget = (TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		if (!ToTarget.IsNearlyZero())
		{
			return ToTarget;
		}
	}

	return ChargeDirection.GetSafeNormal2D().IsNearlyZero() ? FVector::ForwardVector : ChargeDirection.GetSafeNormal2D();
}
