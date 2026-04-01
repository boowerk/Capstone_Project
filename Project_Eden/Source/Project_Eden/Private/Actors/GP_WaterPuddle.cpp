#include "Actors/GP_WaterPuddle.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Utils/GP_BlueprintLibrary.h"

AGP_WaterPuddle::AGP_WaterPuddle()
{
	PrimaryActorTick.bCanEverTick = true; // 크기 축소용 Tick 함수 활성화
	bReplicates = true;

	PuddleCollision = CreateDefaultSubobject<USphereComponent>(TEXT("PuddleCollision"));
	SetRootComponent(PuddleCollision);
	
	// 초기 충돌 채널 세팅 (폰과 장판끼리 겹치도록 설정)
	PuddleCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PuddleCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	PuddleCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	// 장판 액터끼리 오버랩을 감지하기 위해 WorldDynamic과도 겹침 처리
	PuddleCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);

	MovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("MovementComponent"));
	MovementComponent->InitialSpeed = 0.f; // 처음엔 가만히 멈춰있음
	MovementComponent->MaxSpeed = 2000.f;
	MovementComponent->ProjectileGravityScale = 0.f; // 바닥에 붙어있도록 중력 무시
}

void AGP_WaterPuddle::BeginPlay()
{
	Super::BeginPlay();

	CurrentRadius = MaxRadius;
	if (PuddleCollision)
	{
		PuddleCollision->SetSphereRadius(CurrentRadius);
		PuddleCollision->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnPuddleOverlap);
	}
}

void AGP_WaterPuddle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 서버에서만 크기를 줄이고, Replicate를 통해 클라이언트에 반영
	if (HasAuthority() && !bIsBeingAbsorbed)
	{
		CurrentRadius -= ShrinkRatePerSecond * DeltaTime;

		if (CurrentRadius <= MinRadius)
		{
			// 최소 크기 도달 시 틱을 끄고 페이드아웃 후 파괴
			bIsBeingAbsorbed = true; 
			SetActorTickEnabled(false);
			BP_OnFadeOutAndDestroy();
			
			// 1초 뒤 완전히 액터 소멸
			SetLifeSpan(1.0f);
			return;
		}

		// 콜리전 크기 실시간 업데이트
		PuddleCollision->SetSphereRadius(CurrentRadius);
	}
}

void AGP_WaterPuddle::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGP_WaterPuddle, CurrentRadius);
}

void AGP_WaterPuddle::OnPuddleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !IsValid(OtherActor) || OtherActor == this || bIsBeingAbsorbed) return;

	// 1. 다른 물 장판과 겹쳤을 때 (장판 합체)
	if (AGP_WaterPuddle* OtherPuddle = Cast<AGP_WaterPuddle>(OtherActor))
	{
		if (OtherPuddle->bIsBeingAbsorbed) return;

		// 크기가 더 큰 장판이 작은 장판을 흡수함. 크기가 같다면 메모리 주소가 높은 놈이 흡수.
		if (CurrentRadius > OtherPuddle->CurrentRadius || 
		   (CurrentRadius == OtherPuddle->CurrentRadius && this > OtherPuddle))
		{
			// 상대 장판 흡수 처리
			OtherPuddle->bIsBeingAbsorbed = true;
			OtherPuddle->SetActorTickEnabled(false);
			OtherPuddle->PuddleCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 중복 흡수 방지
			
			// 내 크기 키우기 (흡수한 장판 반지름의 50%만큼 추가, 단 MaxRadius 초과 불가)
			CurrentRadius = FMath::Clamp(CurrentRadius + (OtherPuddle->CurrentRadius * 0.5f), MinRadius, MaxRadius);
			
			// 상대 장판 연출 호출 후 파괴
			OtherPuddle->BP_OnAbsorbed();
			OtherPuddle->SetLifeSpan(0.5f);
		}
	}
	// 2. 적(폰)이 장판에 들어왔을 때 (디버프 부여)
	else if (OtherActor->IsA<APawn>() && OtherActor != GetInstigator())
	{
		if (PuddleDebuffEffectClass)
		{
			TArray<AActor*> HitActors;
			HitActors.Add(OtherActor);
			UGP_BlueprintLibrary::ApplyGameplayEffectToActors(GetInstigator(), HitActors, PuddleDebuffEffectClass, 1.0f);
		}
	}
}

void AGP_WaterPuddle::PullTowards(AActor* TargetActor, float PullSpeed)
{
	if (!IsValid(TargetActor) || !IsValid(MovementComponent)) return;

	// 목표물(플레이어)을 향한 방향 계산 후 속도 적용
	FVector Direction = (TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	
	// 장판이 바닥을 타고 오도록 Z축은 무시
	Direction.Z = 0.0f; 
	
	MovementComponent->Velocity = Direction * PullSpeed;
}
