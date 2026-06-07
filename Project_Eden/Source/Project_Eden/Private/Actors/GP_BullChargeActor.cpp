#include "Actors/GP_BullChargeActor.h"

#include "Actors/GP_MatadorBossDecoyActor.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Characters/GP_MatadorBossStateComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "GameplayTags/GP_Tags.h"
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

	BullVisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BullVisualMesh"));
	BullVisualMesh->SetupAttachment(CollisionBox);
	BullVisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// The engine cube gives a visible prototype if no Blueprint mesh has been assigned yet.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		BullVisualMesh->SetStaticMesh(CubeMeshFinder.Object);
		BullVisualMesh->SetRelativeScale3D(FVector(1.8f, 1.0f, 0.8f));
	}

	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageEffectFinder(TEXT("/Game/GAS_Pattern/AbilitySystem/GameplayEffects/Damage/GE_PrimaryDamage"));
	if (DamageEffectFinder.Succeeded())
	{
		DamageEffectClass = DamageEffectFinder.Class;
	}

	PlayerHitEventTag = GPTags::Event::Player::HitReact;
}

void AGP_BullChargeActor::BeginPlay()
{
	Super::BeginPlay();

	SetLifeSpan(MaxChargeLifeSeconds + TelegraphLeadTime + 0.5f);
	CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnChargeOverlap);
	DrawTelegraph();
}

void AGP_BullChargeActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority() || !bChargeStarted || bChargeFinished)
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

	FHitResult MoveHit;
	AddActorWorldOffset(ChargeDirection * ChargeSpeed * DeltaSeconds, true, &MoveHit);
	if (MoveHit.bBlockingHit)
	{
		FinishCharge(false);
	}
}

void AGP_BullChargeActor::InitializeBullCharge(AActor* InTargetActor, AGP_MatadorBossDecoyActor* InDecoyActor, UGP_MatadorBossStateComponent* InStateComponent, const FVector& InChargeDirection)
{
	TargetActor = InTargetActor;
	DecoyActor = InDecoyActor;
	MatadorStateComponent = InStateComponent;

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
}

void AGP_BullChargeActor::StartCharge()
{
	if (bChargeFinished)
	{
		return;
	}

	bChargeStarted = true;
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
		if (!BeginChargeImpact())
		{
			return;
		}

		if (IsValid(MatadorStateComponent.Get()))
		{
			MatadorStateComponent->RecordBullHitDecoy();
		}
		DecoyActor->PlayBreakPresentation();
		FinishCharge(true);
		return;
	}

	if (UGP_BlueprintLibrary::CanApplyCombatEffect(GetOwner(), OtherActor))
	{
		if (!BeginChargeImpact())
		{
			return;
		}

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

void AGP_BullChargeActor::FinishCharge(bool bHitDecoy)
{
	if (bChargeFinished)
	{
		return;
	}

	bChargeFinished = true;
	bImpactHandled = true;
	if (IsValid(CollisionBox))
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	BP_OnBullChargeEnded(bHitDecoy);

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
	DrawDebugLine(World, Start, End, FColor::Orange, false, FMath::Max(0.1f, TelegraphLeadTime), 0, 12.0f);
	DrawDebugBox(World, End, FVector(90.0f, 60.0f, 45.0f), FColor::Orange, false, FMath::Max(0.1f, TelegraphLeadTime), 0, 6.0f);
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
