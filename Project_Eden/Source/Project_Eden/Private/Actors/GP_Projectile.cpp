#include "Actors/GP_Projectile.h"
#include "Components/ShapeComponent.h" 
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameplayTags/GP_Tags.h"
#include "Utils/GP_BlueprintLibrary.h"

#include "GameplayEffect.h"
#include "GameFramework/Pawn.h"

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

void AGP_Projectile::OnProjectileOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// (기존과 동일하게 데미지 적용, 이벤트 전송, 파괴 처리 로직)
	if (!IsValid(OtherActor) || OtherActor == GetInstigator() || OtherActor == this) return;

	if (HasAuthority() && GetInstigator())
	{
		TArray<AActor*> HitActors;
		HitActors.Add(OtherActor);

		if (DamageEffectClass)
		{
			UGP_BlueprintLibrary::ApplyGameplayEffectToActors(GetInstigator(), HitActors, DamageEffectClass, EffectLevel);
		}

		if (HitEventTag.IsValid())
		{
			UGP_BlueprintLibrary::SendGameplayEventToActors(GetInstigator(), HitActors, HitEventTag);
		}
	}

	BP_OnHitEffect(GetActorLocation());

	if (bDestroyOnHit)
	{
		Destroy();
	}
}
