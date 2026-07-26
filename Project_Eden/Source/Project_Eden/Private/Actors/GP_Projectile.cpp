#include "Actors/GP_Projectile.h"
#include "AbilitySystem/Abilities/GP_GameplayAbility.h"
#include "AbilitySystem/Abilities/GP_SkillData.h"
#include "Components/ShapeComponent.h" 
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameplayTags/GP_Tags.h"
#include "Utils/GP_BlueprintLibrary.h"

#include "GameplayEffect.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"

AGP_Projectile::AGP_Projectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	// 1. 빈 씬 컴포넌트를 루트로 지정 (도형은 여기서 안 만듦!)
	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	// 2. 무브먼트 컴포넌트 세팅
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 1500.f;
	ProjectileMovement->MaxSpeed = 1500.f;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;

	HitEventTag = GPTags::Event::Enemy::HitReact;
}

void AGP_Projectile::BeginPlay()
{
	Super::BeginPlay();

	SetLifeSpan(ProjectileLifeSpan);
	if (ProjectileVisualSystem)
	{
		BP_OnProjectileVisualSystemChanged(ProjectileVisualSystem);
	}

	// 3. 블루프린트에서 추가한 ShapeComponent(Box, Sphere, Capsule)를 동적으로 찾아냅니다.
	CollisionComponent = FindComponentByClass<UShapeComponent>();

	if (CollisionComponent)
	{
		// C++에서 콜리전 세팅을 강제로 덮어씌워 휴먼 에러를 방지합니다.
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
		CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		CollisionComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block); 
		CollisionComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);

		// 이벤트 바인딩
		CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnProjectileOverlap);

		// 자해 방지 (무시 처리)
		if (GetInstigator())
		{
			CollisionComponent->IgnoreActorWhenMoving(GetInstigator(), true);
		}
	}
	else
	{
		// 디자이너가 블루프린트에 콜리전을 넣는 것을 깜빡했을 때를 대비한 경고
		UE_LOG(LogTemp, Error, TEXT("[%s] Projectile has no Box, Sphere, or Capsule Component!"), *GetName());
	}
}

void AGP_Projectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGP_Projectile, ProjectileVisualSystem);
}

void AGP_Projectile::SetProjectileVisualSystem(UNiagaraSystem* InProjectileVisualSystem)
{
	if (!InProjectileVisualSystem)
	{
		return;
	}

	ProjectileVisualSystem = InProjectileVisualSystem;
	BP_OnProjectileVisualSystemChanged(ProjectileVisualSystem);
}

void AGP_Projectile::SetImpactVisualActorClass(TSubclassOf<AActor> InImpactVisualActorClass)
{
	ImpactVisualActorClass = InImpactVisualActorClass;
}

void AGP_Projectile::ApplySplashRadiusMultiplier(float RadiusMultiplier)
{
	SplashRadiusMultiplier = FMath::Max(RadiusMultiplier, 0.0f);
}

void AGP_Projectile::SetInfinitePierce(bool bInInfinitePierce)
{
	bInfinitePierce = bInInfinitePierce;
}

void AGP_Projectile::OnRep_ProjectileVisualSystem()
{
	if (ProjectileVisualSystem)
	{
		BP_OnProjectileVisualSystemChanged(ProjectileVisualSystem);
	}
}

void AGP_Projectile::OnProjectileOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority())
	{
		return;
	}

	// (기존과 동일하게 데미지 적용, 이벤트 전송, 파괴 처리 로직)
	if (!IsValid(OtherActor) || OtherActor == GetInstigator() || OtherActor == this) return;
	// Friendly enemies should not consume, destroy, or react to enemy projectiles.
	if (!UGP_BlueprintLibrary::CanApplyCombatEffect(GetInstigator(), OtherActor)) return;

	if (bHasHit)
	{
		return;
	}

	const TWeakObjectPtr<AActor> HitActor(OtherActor);
	if (PiercedActors.Contains(HitActor))
	{
		return;
	}
	PiercedActors.Add(HitActor);


	if (GetInstigator())
	{
		TArray<AActor*> HitActors;
		HitActors.Add(OtherActor);

		UGP_BlueprintLibrary::ApplyGameplayEffectAndEventToActors(GetInstigator(), HitActors, DamageEffectClass, HitEventTag, EffectLevel, SkillData);

		if (SkillData
			&& SkillData->ProjectileImpactDamageMode == EGP_ProjectileImpactDamageMode::DirectAndSplash
			&& SkillData->SplashRadius > 0.0f)
		{
			TArray<AActor*> SplashActors = UGP_BlueprintLibrary::SphereOverlapActorsAtLocation(
				this,
				GetActorLocation(),
				SkillData->SplashRadius * SplashRadiusMultiplier,
				GetInstigator(),
				SkillData->bDrawSplashDebug && UGP_GameplayAbility::IsSkillDebugDrawEnabled());
			SplashActors.Remove(OtherActor);

			UGP_BlueprintLibrary::ApplyGameplayEffectAndEventToActors(
				GetInstigator(),
				SplashActors,
				DamageEffectClass,
				HitEventTag,
				EffectLevel,
				SkillData,
				SkillData->SplashDamageMultiplier);
		}
	}

	MulticastPlayHitEffect(
		GetActorLocation(),
		GetActorRotation(),
		ImpactVisualActorClass,
		SkillData ? SkillData->ImpactRadiusScaleParameterName : NAME_None,
		SplashRadiusMultiplier);

	if (bInfinitePierce)
	{
		return;
	}

	bHasHit = true;
	if (bDestroyOnHit)
	{
		Destroy();
	}
}

void AGP_Projectile::MulticastPlayHitEffect_Implementation(
	const FVector& ImpactLocation,
	const FRotator& ImpactRotation,
	TSubclassOf<AActor> InImpactVisualActorClass,
	FName RadiusScaleParameterName,
	float RadiusScaleMultiplier)
{
	if (InImpactVisualActorClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner();
		SpawnParams.Instigator = GetInstigator();
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* VisualActor = GetWorld()->SpawnActor<AActor>(
			InImpactVisualActorClass,
			ImpactLocation,
			ImpactRotation,
			SpawnParams);

		if (IsValid(VisualActor) && !RadiusScaleParameterName.IsNone())
		{
			TArray<UNiagaraComponent*> NiagaraComponents;
			VisualActor->GetComponents(NiagaraComponents);

			for (UNiagaraComponent* NiagaraComponent : NiagaraComponents)
			{
				if (!IsValid(NiagaraComponent))
				{
					continue;
				}

				bool bHasBaseScale = false;
				const FVector2D AuthoredBaseScale =
					NiagaraComponent->GetVariableVec2(RadiusScaleParameterName, bHasBaseScale);
				const FVector2D BaseScale =
					bHasBaseScale ? AuthoredBaseScale : FVector2D(1.0f, 1.0f);
				NiagaraComponent->DestroyInstanceNotComponent();
				const float SafeMultiplier = FMath::Max(RadiusScaleMultiplier, 0.0f);
				NiagaraComponent->SetVariableVec2(
					RadiusScaleParameterName,
					BaseScale * SafeMultiplier);
				NiagaraComponent->Activate(true);
			}
		}
		return;
	}

	BP_OnHitEffect(ImpactLocation);
}
