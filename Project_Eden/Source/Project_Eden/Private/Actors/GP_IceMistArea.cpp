#include "Actors/GP_IceMistArea.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GP_SkillData.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameplayEffect.h"
#include "Net/UnrealNetwork.h"
#include "Utils/GP_BlueprintLibrary.h"

AGP_IceMistArea::AGP_IceMistArea()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	MovementCollision = CreateDefaultSubobject<USphereComponent>(TEXT("MovementCollision"));
	SetRootComponent(MovementCollision);
	MovementCollision->SetSphereRadius(30.0f);
	MovementCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MovementCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	MovementCollision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);

	AreaCollision = CreateDefaultSubobject<USphereComponent>(TEXT("AreaCollision"));
	AreaCollision->SetupAttachment(MovementCollision);
	AreaCollision->SetSphereRadius(AreaRadius);
	AreaCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AreaCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	AreaCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	AreaCollision->SetGenerateOverlapEvents(true);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = MovementCollision;
	ProjectileMovement->InitialSpeed = LaunchSpeed;
	ProjectileMovement->MaxSpeed = LaunchSpeed;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
}

void AGP_IceMistArea::InitializeIceMist(
	AActor* InSourceActor,
	UGP_SkillData* InSkillData,
	TSubclassOf<UGameplayEffect> InDamageEffectClass,
	TSubclassOf<UGameplayEffect> InSlowEffectClass,
	FGameplayTag InHitEventTag,
	float InEffectLevel,
	float InRadius,
	float InDuration,
	float InDamageInterval,
	const FVector& InLaunchDirection,
	float InLaunchSpeed,
	float InMoveDuration,
	bool bInDrawDebug)
{
	SourceActor = InSourceActor;
	SkillData = InSkillData;
	DamageEffectClass = InDamageEffectClass;
	SlowEffectClass = InSlowEffectClass;
	HitEventTag = InHitEventTag;
	EffectLevel = FMath::Max(InEffectLevel, 1.0f);
	AreaRadius = FMath::Max(InRadius, 0.0f);
	AreaDuration = FMath::Max(InDuration, 0.0f);
	DamageInterval = FMath::Max(InDamageInterval, 0.01f);
	LaunchDirection = InLaunchDirection.GetSafeNormal();
	LaunchSpeed = FMath::Max(InLaunchSpeed, 0.0f);
	MoveDuration = FMath::Max(InMoveDuration, 0.0f);
	bDrawDebug = bInDrawDebug;

	if (AreaCollision)
	{
		AreaCollision->SetSphereRadius(AreaRadius);
	}
}

void AGP_IceMistArea::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	if (!IsValid(SourceActor))
	{
		Destroy();
		return;
	}

	AreaCollision->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandleAreaBeginOverlap);
	AreaCollision->OnComponentEndOverlap.AddDynamic(this, &ThisClass::HandleAreaEndOverlap);
	ProjectileMovement->OnProjectileStop.AddDynamic(this, &ThisClass::HandleProjectileStopped);
	ProjectileMovement->InitialSpeed = LaunchSpeed;
	ProjectileMovement->MaxSpeed = LaunchSpeed;
	ProjectileMovement->Velocity = LaunchDirection * LaunchSpeed;
	AreaCollision->UpdateOverlaps();

	TArray<AActor*> InitialActors;
	AreaCollision->GetOverlappingActors(InitialActors);
	for (AActor* InitialActor : InitialActors)
	{
		AddAffectedActor(InitialActor);
	}

	GetWorldTimerManager().SetTimer(
		DamageTimerHandle,
		this,
		&ThisClass::ApplyDamageTick,
		DamageInterval,
		true,
		DamageInterval);

	if (MoveDuration <= 0.0f)
	{
		StopMistMovement();
	}
	else
	{
		GetWorldTimerManager().SetTimer(
			MovementTimerHandle,
			this,
			&ThisClass::StopMistMovement,
			MoveDuration,
			false);
	}

	SetLifeSpan(AreaDuration);
}

void AGP_IceMistArea::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGP_IceMistArea, bMovementStopped);
}

void AGP_IceMistArea::HandleProjectileStopped(const FHitResult& ImpactResult)
{
	StopMistMovement();
}

void AGP_IceMistArea::StopMistMovement()
{
	if (bMovementStopped)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(MovementTimerHandle);
	bMovementStopped = true;
	ApplyStoppedMovementState();
	ForceNetUpdate();
}

void AGP_IceMistArea::OnRep_MovementStopped()
{
	if (bMovementStopped)
	{
		ApplyStoppedMovementState();
	}
}

void AGP_IceMistArea::ApplyStoppedMovementState()
{
	ProjectileMovement->StopMovementImmediately();
	ProjectileMovement->Deactivate();
	MovementCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AGP_IceMistArea::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(DamageTimerHandle);
	GetWorldTimerManager().ClearTimer(MovementTimerHandle);
	RemoveAllSlowEffects();
	Super::EndPlay(EndPlayReason);
}

void AGP_IceMistArea::HandleAreaBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	AddAffectedActor(OtherActor);
}

void AGP_IceMistArea::HandleAreaEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	RemoveAffectedActor(OtherActor);
}

void AGP_IceMistArea::AddAffectedActor(AActor* OtherActor)
{
	if (!UGP_BlueprintLibrary::CanApplyCombatEffect(SourceActor, OtherActor) ||
		SlowEffectHandles.Contains(OtherActor))
	{
		return;
	}

	UAbilitySystemComponent* SourceASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourceActor);
	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
	if (!IsValid(SourceASC) || !IsValid(TargetASC))
	{
		return;
	}

	FActiveGameplayEffectHandle SlowHandle;
	if (SlowEffectClass)
	{
		FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
		Context.AddInstigator(SourceActor, SourceActor);
		const FGameplayEffectSpecHandle Spec =
			SourceASC->MakeOutgoingSpec(SlowEffectClass, EffectLevel, Context);
		if (Spec.IsValid())
		{
			SlowHandle = SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
		}
	}

	SlowEffectHandles.Add(OtherActor, SlowHandle);
}

void AGP_IceMistArea::RemoveAffectedActor(AActor* OtherActor)
{
	const FActiveGameplayEffectHandle* SlowHandle = SlowEffectHandles.Find(OtherActor);
	if (!SlowHandle)
	{
		return;
	}

	if (SlowHandle->IsValid())
	{
		if (UAbilitySystemComponent* TargetASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor))
		{
			TargetASC->RemoveActiveGameplayEffect(*SlowHandle);
		}
	}

	SlowEffectHandles.Remove(OtherActor);
}

void AGP_IceMistArea::ApplyDamageTick()
{
	if (!IsValid(SourceActor))
	{
		return;
	}

	const TArray<AActor*> HitActors = UGP_BlueprintLibrary::SphereOverlapActorsAtLocation(
		SourceActor,
		AreaCollision->GetComponentLocation(),
		AreaRadius,
		SourceActor,
		bDrawDebug);

	UGP_BlueprintLibrary::ApplyGameplayEffectAndEventToActors(
		SourceActor,
		HitActors,
		DamageEffectClass,
		HitEventTag,
		EffectLevel,
		SkillData);
}

void AGP_IceMistArea::RemoveAllSlowEffects()
{
	TArray<TWeakObjectPtr<AActor>> AffectedActors;
	SlowEffectHandles.GetKeys(AffectedActors);
	for (const TWeakObjectPtr<AActor>& AffectedActor : AffectedActors)
	{
		RemoveAffectedActor(AffectedActor.Get());
	}
	SlowEffectHandles.Reset();
}
