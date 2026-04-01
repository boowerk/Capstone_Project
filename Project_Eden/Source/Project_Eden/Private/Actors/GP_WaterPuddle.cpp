#include "Actors/GP_WaterPuddle.h"

#include "GameplayEffect.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Utils/GP_BlueprintLibrary.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "Components/DecalComponent.h"

AGP_WaterPuddle::AGP_WaterPuddle()
{
	PrimaryActorTick.bCanEverTick = false; 
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
	
	PuddleDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("PuddleDecal"));
	PuddleDecal->SetupAttachment(RootComponent);
    
	// 데칼이 바닥 방향(아래)을 향하도록 회전 (Pitch -90도)
	PuddleDecal->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
    
	// 기본 투사 거리 설정
	PuddleDecal->DecalSize = FVector(128.0f, MaxRadius, MaxRadius);
}

void AGP_WaterPuddle::BeginPlay()
{
	Super::BeginPlay();
	
	if (DecalMaterial)
	{
		DecalDynamicMaterial = UMaterialInstanceDynamic::Create(DecalMaterial, this);
		PuddleDecal->SetDecalMaterial(DecalDynamicMaterial);
	}

	CurrentRadius = MaxRadius;
	StartingRadiusForLerp = MaxRadius;
	
	if (PuddleCollision)
	{
		PuddleCollision->SetSphereRadius(CurrentRadius);
		PuddleCollision->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnPuddleOverlap);
	}
	
	if (HasAuthority())
	{
		StartTime = GetWorld()->GetTimeSeconds();
		EndTime = StartTime + BaseDuration;

		// 0.1초 10Hz 마다 콜리전 업데이트 함수 실행 하게 함 이게 Tick보다 훨씬 가볍다한다 - 슝민
		GetWorld()->GetTimerManager().SetTimer(PuddleUpdateTimer, this, &ThisClass::UpdatePuddleState, 0.1f, true);
	}
}

void AGP_WaterPuddle::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGP_WaterPuddle, EndTime);
}

void AGP_WaterPuddle::UpdatePuddleState()
{
	if (bIsBeingAbsorbed) return;

	if (bIsMovingToDestination)
	{
		FVector CurrentLoc = GetActorLocation();
		FVector DirToTarget = DestinationLoc - CurrentLoc;
		
		// 1. 거리가 아주 가깝거나 (50 유닛 이하)
		// 2. 내적(DotProduct)이 0 이하인 경우 (장판이 목표 지점을 지나쳐서 방향이 뒤집힌 경우 = Overshoot 방지)
		if (DirToTarget.SizeSquared() <= 2500.f || FVector::DotProduct(DirToTarget, MovementComponent->Velocity) <= 0.f)
		{
			MovementComponent->Velocity = FVector::ZeroVector; // 브레이크
			bIsMovingToDestination = false;
		}
	}
	
	float CurrentTime = GetWorld()->GetTimeSeconds();
	
	// 남은 시간 비율 계산: 1.0에서 시작해 0.0으로 끝남
	float TotalTime = EndTime - StartTime;
	float Alpha = 0.0f;
	
	if (TotalTime > 0.0f)
	{
		Alpha = FMath::Clamp((EndTime - CurrentTime) / TotalTime, 0.0f, 1.0f);
	}

	// 남은 비율 Alpha에 따라 MinRadius ~ StartingRadius 사이의 값을 보간
	CurrentRadius = FMath::Lerp(MinRadius, StartingRadiusForLerp, Alpha);
	PuddleCollision->SetSphereRadius(CurrentRadius);
	

	// 데칼 크기 업데이트 (Y, Z축이 평면상의 크기임)
	// 데칼의 사이즈는 반지름이 아닌 전체 영역이므로 CurrentRadius를 그대로 사용하거나 보정함
	PuddleDecal->DecalSize = FVector(128.0f, CurrentRadius, CurrentRadius);

	// 시간이 다 되어 사라질 때 페이드 아웃 연출 (머티리얼 파라미터 조절)
	if (DecalDynamicMaterial && (EndTime - GetWorld()->GetTimeSeconds()) < 1.0f)
	{
		float Opacity = FMath::Clamp(EndTime - GetWorld()->GetTimeSeconds(), 0.0f, 1.0f);
		DecalDynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), Opacity);
	}
	
	// 시간이 다 되면 파괴 처리
	if (CurrentTime >= EndTime)
	{
		bIsBeingAbsorbed = true;
		GetWorld()->GetTimerManager().ClearTimer(PuddleUpdateTimer);
		
		BP_OnFadeOutAndDestroy();
		SetLifeSpan(1.0f);
	}
}

// AI가 마음대로 짠 로직 제거하고 다시 짬 - 슝민
void AGP_WaterPuddle::OnPuddleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !IsValid(OtherActor) || OtherActor == this || bIsBeingAbsorbed) return;
	
	if (AGP_WaterPuddle* OtherPuddle = Cast<AGP_WaterPuddle>(OtherActor))
	{
		if (OtherPuddle->bIsBeingAbsorbed) return;

		if (CurrentRadius > OtherPuddle->CurrentRadius || (CurrentRadius == OtherPuddle->CurrentRadius && this > OtherPuddle))
		{
			// 흡수되는 쪽 타이머 정리
			OtherPuddle->bIsBeingAbsorbed = true;
			OtherPuddle->GetWorld()->GetTimerManager().ClearTimer(OtherPuddle->PuddleUpdateTimer);
			OtherPuddle->PuddleCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			
			// 흡수할 때 시간을 더해줌 : 흡수되는 쪽의 남은 시간의 50퍼 흡수
			float OtherRemainingTime = FMath::Max(0.0f, OtherPuddle->EndTime - GetWorld()->GetTimeSeconds());
			
			// 내 현재 시점부터 다시 보간 시작하기 위한 갱신
			StartTime = GetWorld()->GetTimeSeconds();
			StartingRadiusForLerp = FMath::Clamp(CurrentRadius + (OtherPuddle->CurrentRadius * 0.5f), MinRadius, MaxRadius);
			EndTime = StartTime + FMath::Max(EndTime - StartTime, OtherRemainingTime * 0.5f); // 시간 연장

			// 연출 및 파괴
			OtherPuddle->BP_OnAbsorbed();
			OtherPuddle->SetLifeSpan(0.5f);
		}
	}
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

void AGP_WaterPuddle::OnRep_EndTime()
{
	// 클라이언트에서 EndTime(종료 시간)을 서버로부터 받았을 때 블루프린트로 알려줍니다.
	float RemainingTime = FMath::Max(0.0f, EndTime - GetWorld()->GetTimeSeconds());
	BP_OnEndTimeUpdated(RemainingTime);
}
void AGP_WaterPuddle::CommandPullTowards(AActor* TargetActor, float PullSpeed)
{
	if (!IsValid(TargetActor) || !IsValid(MovementComponent)) return;

	// 목표물(플레이어)을 향한 방향 계산 후 속도 적용
	FVector Direction = (TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	
	// 장판이 바닥을 타고 오도록 Z축은 무시
	Direction.Z = 0.0f; 
	
	MovementComponent->Velocity = Direction * PullSpeed;
}

void AGP_WaterPuddle::CommandAttackTarget()
{
	// (Q 스킬 기획) 장판 안의 적에게 물고기가 튀어나오는 로직을 나중에 여기에 구현할 예정
}

void AGP_WaterPuddle::CommandMoveToLocation(const FVector& TargetLocation, float MoveSpeed)
{
	if (!IsValid(MovementComponent)) return;

	DestinationLoc = TargetLocation;
	DestinationLoc.Z = GetActorLocation().Z; // 장판이 바닥을 유지하도록 Z축 고정
	bIsMovingToDestination = true;

	// 기존 유도탄 기능이 켜져있을 수 있으니 끄기
	MovementComponent->bIsHomingProjectile = false; 
	MovementComponent->MaxSpeed = MoveSpeed;

	// 목표를 향한 방향 벡터 계산 및 속도 적용
	FVector Direction = (DestinationLoc - GetActorLocation()).GetSafeNormal();
	MovementComponent->Velocity = Direction * MoveSpeed;
}
