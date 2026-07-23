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
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
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
	CollisionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Overlap);
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
		UNiagaraComponent* SpawnedEffect = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			BullSpawnEffect,
			GetActorLocation() + BullSpawnEffectOffset,
			GetActorRotation(),
			BullSpawnEffectScale,
			true,
			true,
			ENCPoolMethod::AutoRelease);

		if (IsValid(SpawnedEffect))
		{
			// NS_Free_Magic_Area2 exposes separate colors for every visible emitter.
			SpawnedEffect->SetVariableLinearColor(TEXT("User.Color_Ray"), BullSpawnEffectColor);
			SpawnedEffect->SetVariableLinearColor(TEXT("User.Color_Smoke"), BullSpawnEffectColor);
			SpawnedEffect->SetVariableLinearColor(TEXT("User.Color_Sparks1"), BullSpawnEffectColor);
			SpawnedEffect->SetVariableLinearColor(TEXT("User.Color_Sparks2"), BullSpawnEffectColor);
			SpawnedEffect->SetVariableLinearColor(TEXT("User.Color_Spiral1"), BullSpawnEffectColor);
			SpawnedEffect->SetVariableLinearColor(TEXT("User.Color_Trace"), BullSpawnEffectColor);
			SpawnedEffect->SetVariableLinearColor(TEXT("User.Color_Wave"), BullSpawnEffectColor);
		}
	}
	if (bShowDebugVisuals)
	{
		DrawTelegraph();
	}
}

void AGP_BullChargeActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGP_BullChargeActor, ChargeState);
}

void AGP_BullChargeActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority() || bChargeFinished)
	{
		return;
	}

	TotalActionElapsedSeconds += FMath::Max(0.0f, DeltaSeconds);
	if (TotalActionElapsedSeconds >= FMath::Max(0.1f, MaximumTotalActionLifeSeconds))
	{
		// Redirect가 단계별 lifespan을 반복 갱신해도 고장 난 패턴은 유한 시간 안에 반드시 정리한다.
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[MatadorBull] Absolute action lifetime reached; forcing cleanup. Bull=%s Elapsed=%.2f State=%d"),
			*GetNameSafe(this),
			TotalActionElapsedSeconds,
			static_cast<int32>(ChargeState));
		FinishCharge(false);
		return;
	}

	if (!bChargeStarted)
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
	const float TurnInterpSpeed = ChargeState == EGPMatadorBullChargeState::RedirectedByDecoyToPlayer
		? FMath::Max(0.0f, PlayerRedirectTurnInterpSpeed)
		: FMath::Max(0.0f, HomingTurnSpeedDegrees / 90.0f);
	ChargeDirection = FMath::VInterpNormalRotationTo(ChargeDirection, DesiredDirection, DeltaSeconds, TurnInterpSpeed);
	if (FMath::Acos(FMath::Clamp(FVector::DotProduct(ChargeDirection, DesiredDirection), -1.0f, 1.0f)) <= MaxTurnRadians)
	{
		ChargeDirection = DesiredDirection;
	}

	const FVector PreviousLocation = GetActorLocation();
	FHitResult MoveHit;
	AddActorWorldOffset(ChargeDirection * ChargeSpeed * DeltaSeconds, true, &MoveHit);
	if (MoveHit.bBlockingHit)
	{
		if (ChargeState == EGPMatadorBullChargeState::ChargingToDecoy && IsBlockingHitSafeNearDecoy(MoveHit))
		{
			const float DistanceToDecoy = FVector::Dist2D(GetActorLocation(), DecoyActor->GetActorLocation());
			UE_LOG(LogTemp, Warning, TEXT("[MatadorBull] Blocking hit near decoy; trying redirect instead of finishing. Bull=%s Hit=%s Distance=%.1f"),
				*GetNameSafe(this),
				*GetNameSafe(MoveHit.GetActor()),
				DistanceToDecoy);
			if (!StartDecoyCircleRedirect())
			{
				TryHandleDecoyProximity(&PreviousLocation);
			}
			return;
		}

		FinishCharge(false);
		return;
	}
	SetActorRotation(ChargeDirection.Rotation());

	TryHandleDecoyProximity(&PreviousLocation);
}

void AGP_BullChargeActor::Destroyed()
{
	GetWorldTimerManager().ClearTimer(ChargeStartTimerHandle);
	// Lifespan 만료·외부 Destroy·owner 정리에서도 active pointer와 loose tag를 남기지 않는다.
	ClearRegisteredBullState();
	Super::Destroyed();
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
		UE_LOG(LogTemp, Log, TEXT("[MatadorBull] Redirect toward decoy rejected. Bull=%s Redirector=%s HasAuthority=%d State=%d ChargeStarted=%d Finished=%d AlreadyRedirectedByPlayer=%d Decoy=%s"),
			*GetNameSafe(this),
			*GetNameSafe(RedirectingActor),
			HasAuthority() ? 1 : 0,
			static_cast<int32>(ChargeState),
			bChargeStarted ? 1 : 0,
			bChargeFinished ? 1 : 0,
			bRedirectedByPlayer ? 1 : 0,
			*GetNameSafe(DecoyActor.Get()));
		return false;
	}

	bRedirectedByPlayer = true;
	TargetActor = DecoyActor.Get();
	const FVector ToDecoy = (DecoyActor->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (!ToDecoy.IsNearlyZero())
	{
		ChargeDirection = ToDecoy;
		SetActorRotation(ChargeDirection.Rotation());
	}
	const float DistanceToDecoy = FVector::Dist2D(GetActorLocation(), DecoyActor->GetActorLocation());
	const float PlayerCounterLifeSeconds = DistanceToDecoy / FMath::Max(1.0f, ChargeSpeed) + PlayerCounterLifeExtensionSeconds;
	SetLifeSpan(FMath::Max(0.1f, PlayerCounterLifeSeconds));
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

	if (!bChargeStarted
		|| (ChargeState != EGPMatadorBullChargeState::RedirectedByDecoyToPlayer
			&& ChargeState != EGPMatadorBullChargeState::ChargingToPlayer
			&& ChargeState != EGPMatadorBullChargeState::CirclingDecoyToPlayer))
	{
		return false;
	}

	return true;
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

			UE_LOG(LogTemp, Warning, TEXT("[MatadorBull] Decoy overlap could not start redirect yet; keeping bull alive for retry. Bull=%s Decoy=%s Player=%s"),
				*GetNameSafe(this),
				*GetNameSafe(DecoyActor.Get()),
				*GetNameSafe(PlayerTargetActor.Get()));
			return;
		}

		if (ChargeState == EGPMatadorBullChargeState::CirclingDecoyToPlayer
			|| ChargeState == EGPMatadorBullChargeState::RedirectedByDecoyToPlayer
			|| ChargeState == EGPMatadorBullChargeState::ChargingToPlayer)
		{
			UE_LOG(LogTemp, Verbose, TEXT("[MatadorBull] Ignored decoy overlap during redirect. Bull=%s State=%d Decoy=%s"),
				*GetNameSafe(this),
				static_cast<int32>(ChargeState),
				*GetNameSafe(DecoyActor.Get()));
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

		ApplyBullImpactToActor(OtherActor);
		return;
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

		UE_LOG(LogTemp, Warning, TEXT("[MatadorBull] Decoy proximity could not start redirect yet; keeping bull alive for retry. Bull=%s Decoy=%s Player=%s"),
			*GetNameSafe(this),
			*GetNameSafe(DecoyActor.Get()),
			*GetNameSafe(PlayerTargetActor.Get()));
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
	DecoyRedirectCurveAlpha = 0.0f;

	const FVector ToPlayer = (PlayerTarget->GetActorLocation() - DecoyActor->GetActorLocation()).GetSafeNormal2D();
	const FVector StartDirection = DecoyCircleStartOffset.GetSafeNormal2D();
	const FVector SafeToPlayer = ToPlayer.IsNearlyZero() ? GetActorForwardVector().GetSafeNormal2D() : ToPlayer;
	const FVector SafeChargeDirection = ChargeDirection.GetSafeNormal2D().IsNearlyZero() ? -StartDirection : ChargeDirection.GetSafeNormal2D();
	const FVector LeftSideDirection = FRotator(0.0f, 90.0f, 0.0f).RotateVector(SafeToPlayer).GetSafeNormal2D();
	const FVector RightSideDirection = FRotator(0.0f, -90.0f, 0.0f).RotateVector(SafeToPlayer).GetSafeNormal2D();
	const bool bUseLeftOuterSide = FVector::DotProduct(LeftSideDirection, StartDirection) <= FVector::DotProduct(RightSideDirection, StartDirection);
	DecoyCircleDirectionSign = bUseLeftOuterSide ? 1.0f : -1.0f;
	const FVector SideDirection = bUseLeftOuterSide ? LeftSideDirection : RightSideDirection;

	DecoyRedirectCurveP0 = GetActorLocation();
	DecoyRedirectCurveP3 = DecoyActor->GetActorLocation() + SafeToPlayer * DecoyCircleRadius;
	DecoyRedirectCurveP3.Z = DecoyRedirectCurveP0.Z;
	DecoyRedirectCurveP1 =
		DecoyRedirectCurveP0
		+ SafeChargeDirection * DecoyCircleRadius * 0.65f
		+ SideDirection * DecoyCircleRadius * DecoyRedirectOuterSideStrengthP1;
	DecoyRedirectCurveP2 =
		DecoyRedirectCurveP3
		- SafeToPlayer * DecoyCircleRadius * 0.9f
		+ SideDirection * DecoyCircleRadius * DecoyRedirectOuterSideStrengthP2;

	DecoyRedirectCurveLength = 0.0f;
	FVector PreviousCurvePoint = DecoyRedirectCurveP0;
	for (int32 SampleIndex = 1; SampleIndex <= 16; ++SampleIndex)
	{
		const float SampleAlpha = static_cast<float>(SampleIndex) / 16.0f;
		const FVector CurvePoint = EvaluateDecoyRedirectCurve(SampleAlpha);
		DecoyRedirectCurveLength += FVector::Dist(PreviousCurvePoint, CurvePoint);
		PreviousCurvePoint = CurvePoint;
	}
	DecoyRedirectCurveLength = FMath::Max(1.0f, DecoyRedirectCurveLength);

	TargetActor = nullptr;
	const float EstimatedCurveSeconds = DecoyRedirectCurveLength / FMath::Max(1.0f, ChargeSpeed);
	const float EstimatedPlayerLegSeconds = FVector::Dist2D(DecoyRedirectCurveP3, PlayerTarget->GetActorLocation()) / FMath::Max(1.0f, ChargeSpeed);
	SetLifeSpan(FMath::Max(0.1f, EstimatedCurveSeconds + EstimatedPlayerLegSeconds + PlayerCounterLifeExtensionSeconds));
	SetChargeState(EGPMatadorBullChargeState::CirclingDecoyToPlayer);
	DecoyActor->PlayBullRedirectPresentation(this, PlayerTarget);

	UE_LOG(LogTemp, Log, TEXT("[MatadorBull] Decoy circle redirect started. Bull=%s Decoy=%s Player=%s Arc=%.1f Radius=%.1f Sign=%.0f Life=%.2f"),
		*GetNameSafe(this),
		*GetNameSafe(DecoyActor.Get()),
		*GetNameSafe(PlayerTarget),
		DecoyRedirectArcDegrees,
		DecoyCircleRadius,
		DecoyCircleDirectionSign,
		GetLifeSpan());

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

	const float DistanceStep = FMath::Max(0.0f, ChargeSpeed) * FMath::Max(0.0f, DeltaSeconds);
	const float AlphaStep = DistanceStep / FMath::Max(1.0f, DecoyRedirectCurveLength);
	const float NextAlpha = FMath::Min(1.0f, DecoyRedirectCurveAlpha + AlphaStep);
	const float LookAheadAlpha = FMath::Min(1.0f, NextAlpha + DecoyRedirectLookAheadAlpha);
	const FVector CurveTargetDirection = (EvaluateDecoyRedirectCurve(LookAheadAlpha) - GetActorLocation()).GetSafeNormal2D();
	const FVector CurveTangentDirection = EvaluateDecoyRedirectCurveTangent(NextAlpha).GetSafeNormal2D();
	const float TangentWeight = FMath::Clamp(DecoyRedirectTangentWeight, 0.0f, 1.0f);
	FVector DesiredDirection = CurveTargetDirection * (1.0f - TangentWeight) + CurveTangentDirection * TangentWeight;
	DesiredDirection = DesiredDirection.GetSafeNormal2D();
	if (!DesiredDirection.IsNearlyZero())
	{
		ChargeDirection = DecoyRedirectTurnInterpSpeed > 0.0f
			? FMath::VInterpNormalRotationTo(ChargeDirection, DesiredDirection, DeltaSeconds, DecoyRedirectTurnInterpSpeed)
			: DesiredDirection;
		if (ChargeDirection.IsNearlyZero())
		{
			ChargeDirection = DesiredDirection;
		}
	}

	const FVector PreviousLocation = GetActorLocation();
	const FVector PreviousCurveLocation = EvaluateDecoyRedirectCurve(DecoyRedirectCurveAlpha);
	const FVector NextCurveLocation = EvaluateDecoyRedirectCurve(NextAlpha);
	const FVector CurveMoveDelta = NextCurveLocation - PreviousCurveLocation;
	const FVector CurveMoveDirection = CurveMoveDelta.GetSafeNormal2D();
	if (!CurveMoveDirection.IsNearlyZero())
	{
		ChargeDirection = CurveMoveDirection;
	}
	FHitResult MoveHit;
	AddActorWorldOffset(CurveMoveDelta, true, &MoveHit);
	if (MoveHit.bBlockingHit)
	{
		if (IsBlockingHitSafeNearDecoy(MoveHit))
		{
			UE_LOG(LogTemp, Verbose, TEXT("[MatadorBull] Ignored blocking hit near decoy during curve redirect. Bull=%s Hit=%s Decoy=%s"),
				*GetNameSafe(this),
				*GetNameSafe(MoveHit.GetActor()),
				*GetNameSafe(DecoyActor.Get()));
			SetActorLocation(NextCurveLocation, false);
			SetActorRotation(ChargeDirection.Rotation());
			DecoyRedirectCurveAlpha = NextAlpha;
		}
		else
		{
			FinishCharge(false);
		}
		return;
	}
	SetActorRotation(ChargeDirection.Rotation());

	DecoyRedirectCurveAlpha = NextAlpha;

	if (TryHandleDecoyProximity(&PreviousLocation))
	{
		return;
	}

	if (bShowDebugVisuals)
	{
		for (int32 SampleIndex = 0; SampleIndex < 12; ++SampleIndex)
		{
			const float AlphaA = static_cast<float>(SampleIndex) / 12.0f;
			const float AlphaB = static_cast<float>(SampleIndex + 1) / 12.0f;
			DrawDebugLine(GetWorld(), EvaluateDecoyRedirectCurve(AlphaA), EvaluateDecoyRedirectCurve(AlphaB), FColor::Cyan, false, 0.05f, 0, 4.0f);
		}
	}

	if (DecoyRedirectCurveAlpha >= 1.0f - KINDA_SMALL_NUMBER)
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

FVector AGP_BullChargeActor::EvaluateDecoyRedirectCurve(float Alpha) const
{
	const float T = FMath::Clamp(Alpha, 0.0f, 1.0f);
	const float OneMinusT = 1.0f - T;
	return DecoyRedirectCurveP0 * OneMinusT * OneMinusT * OneMinusT
		+ DecoyRedirectCurveP1 * 3.0f * OneMinusT * OneMinusT * T
		+ DecoyRedirectCurveP2 * 3.0f * OneMinusT * T * T
		+ DecoyRedirectCurveP3 * T * T * T;
}

FVector AGP_BullChargeActor::EvaluateDecoyRedirectCurveTangent(float Alpha) const
{
	const float T = FMath::Clamp(Alpha, 0.0f, 1.0f);
	const float OneMinusT = 1.0f - T;
	return (DecoyRedirectCurveP1 - DecoyRedirectCurveP0) * 3.0f * OneMinusT * OneMinusT
		+ (DecoyRedirectCurveP2 - DecoyRedirectCurveP1) * 6.0f * OneMinusT * T
		+ (DecoyRedirectCurveP3 - DecoyRedirectCurveP2) * 3.0f * T * T;
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
		LockedRedirectDirection = ToPlayer;
	}
	else
	{
		LockedRedirectDirection = ChargeDirection.GetSafeNormal2D().IsNearlyZero()
			? GetActorForwardVector().GetSafeNormal2D()
			: ChargeDirection.GetSafeNormal2D();
	}
	const float DistanceToPlayer = FVector::Dist2D(GetActorLocation(), PlayerTarget->GetActorLocation());
	const float PlayerRedirectLifeSeconds = DistanceToPlayer / FMath::Max(1.0f, ChargeSpeed) + DecoyRedirectLifeExtensionSeconds;
	SetLifeSpan(FMath::Max(0.1f, PlayerRedirectLifeSeconds));
	SetChargeState(EGPMatadorBullChargeState::RedirectedByDecoyToPlayer);
	DecoyActor->PlayBullRedirectPresentation(this, PlayerTarget);
	BP_OnBullRedirected(DecoyActor.Get(), PlayerTarget);
	if (bShowDebugVisuals)
	{
		DrawDebugLine(
			GetWorld(),
			GetActorLocation() + FVector(0.0f, 0.0f, 80.0f),
			GetActorLocation() + LockedRedirectDirection * 1600.0f + FVector(0.0f, 0.0f, 80.0f),
			FColor::Yellow,
			false,
			2.0f,
			0,
			10.0f);
	}

	UE_LOG(LogTemp, Log, TEXT("[MatadorBull] Decoy redirected bull toward player. Bull=%s Decoy=%s Player=%s CurrentDirection=%s LockedDirection=%s Life=%.2f"),
		*GetNameSafe(this),
		*GetNameSafe(DecoyActor.Get()),
		*GetNameSafe(PlayerTarget),
		*ChargeDirection.ToCompactString(),
		*LockedRedirectDirection.ToCompactString(),
		GetLifeSpan());

	return true;
}

bool AGP_BullChargeActor::CanRecordDecoyHit() const
{
	return ChargeState == EGPMatadorBullChargeState::RedirectedByPlayerToDecoy;
}

bool AGP_BullChargeActor::IsActorRelatedToDecoy(AActor* Actor) const
{
	AActor* CurrentDecoyActor = DecoyActor.Get();
	if (!IsValid(Actor) || !IsValid(CurrentDecoyActor))
	{
		return false;
	}

	return Actor == CurrentDecoyActor
		|| Actor->GetOwner() == CurrentDecoyActor
		|| Actor->GetAttachParentActor() == CurrentDecoyActor
		|| CurrentDecoyActor->GetOwner() == Actor
		|| CurrentDecoyActor->GetAttachParentActor() == Actor;
}

bool AGP_BullChargeActor::IsBlockingHitSafeNearDecoy(const FHitResult& Hit) const
{
	AActor* CurrentDecoyActor = DecoyActor.Get();
	if (!IsValid(CurrentDecoyActor))
	{
		return false;
	}

	if (IsActorRelatedToDecoy(Hit.GetActor()))
	{
		return true;
	}

	const float DistanceToDecoy = FVector::Dist2D(GetActorLocation(), CurrentDecoyActor->GetActorLocation());
	const float SafeRadius = FMath::Max3(DecoyRedirectRadius, DecoyRedirectArcRadius, DecoyRedirectCollisionGraceRadius);
	return DistanceToDecoy <= SafeRadius;
}

void AGP_BullChargeActor::ApplyBullImpactToActor(AActor* HitActor)
{
	if (!HasAuthority() || !IsValid(HitActor))
	{
		return;
	}

	for (const TWeakObjectPtr<AActor>& PreviousHitActor : BullImpactHitActors)
	{
		if (PreviousHitActor.Get() == HitActor)
		{
			return;
		}
	}
	BullImpactHitActors.Add(TWeakObjectPtr<AActor>(HitActor));

	const FVector ImpactDirection = ChargeDirection.GetSafeNormal2D().IsNearlyZero()
		? GetActorForwardVector().GetSafeNormal2D()
		: ChargeDirection.GetSafeNormal2D();

	AActor* InstigatorActor = GetOwner();
	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InstigatorActor);
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
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

	if (ACharacter* HitCharacter = Cast<ACharacter>(HitActor))
	{
		const FVector LaunchVelocity =
			ImpactDirection * FMath::Max(0.0f, BullHitKnockbackHorizontalSpeed)
			+ FVector::UpVector * FMath::Max(0.0f, BullHitKnockbackVerticalSpeed);
		HitCharacter->LaunchCharacter(LaunchVelocity, true, true);
	}

	TArray<AActor*> HitActors;
	HitActors.Add(HitActor);
	UGP_BlueprintLibrary::SendGameplayEventToActors(InstigatorActor, HitActors, PlayerHitEventTag);

	UE_LOG(LogTemp, Log, TEXT("[MatadorBull] Player impact. Bull=%s Target=%s Direction=%s Damage=%.1f Knockback=%.1f"),
		*GetNameSafe(this),
		*GetNameSafe(HitActor),
		*ImpactDirection.ToCompactString(),
		BullHitBaseDamage,
		BullHitKnockbackHorizontalSpeed);
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

	ClearRegisteredBullState();

	if (HasAuthority())
	{
		Destroy();
	}
}

void AGP_BullChargeActor::ClearRegisteredBullState()
{
	if (HasAuthority()
		&& IsValid(MatadorStateComponent.Get())
		&& MatadorStateComponent->GetActiveBullActor() == this)
	{
		// 새 황소가 이미 등록된 경우에는 이전 actor의 늦은 Destroy가 새 상태를 지우지 않는다.
		MatadorStateComponent->RegisterActiveBullActor(nullptr);
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
	if (ChargeState == EGPMatadorBullChargeState::RedirectedByDecoyToPlayer)
	{
		const FVector LockedDirection = LockedRedirectDirection.GetSafeNormal2D();
		return LockedDirection.IsNearlyZero() ? FVector::ForwardVector : LockedDirection;
	}

	if (ChargeState == EGPMatadorBullChargeState::RedirectedByPlayerToDecoy)
	{
		const FVector LockedDirection = ChargeDirection.GetSafeNormal2D();
		return LockedDirection.IsNearlyZero() ? FVector::ForwardVector : LockedDirection;
	}

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
