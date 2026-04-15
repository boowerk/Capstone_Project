#include "Actors/GP_WaterPuddle.h"

#include "Components/DecalComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Utils/GP_BlueprintLibrary.h"

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
	PuddleDecal->DecalSize = FVector(128.0f, StartRadius, StartRadius);
}

void AGP_WaterPuddle::InitializeMovement(AActor* Caster)
{
	if (!Caster || !IsValid(MovementComponent)) return;

	FVector CasterLocation = Caster->GetActorLocation();
	FVector PuddleLocation = GetActorLocation();

	// Z축 높이를 맞추기
	CasterLocation.Z = PuddleLocation.Z;

	// 시전자 to 장판 위치 방향
	FVector MoveDirection = (PuddleLocation - CasterLocation).GetSafeNormal();

	// 장판의 회전과 속도를 해당 방향으로 설정
	SetActorRotation(MoveDirection.Rotation());
	MovementComponent->Velocity = MoveDirection * InitialForwardSpeed;
}
void AGP_WaterPuddle::BeginPlay()
{
	Super::BeginPlay();
	UMaterialInterface* BaseMaterial = PuddleDecal->GetDecalMaterial();
	if (BaseMaterial && PuddleDecal)
	{
		DecalDynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		PuddleDecal->SetDecalMaterial(DecalDynamicMaterial);

		float TempValue;
		if (!DecalDynamicMaterial->GetScalarParameterValue(TEXT("Opacity"), TempValue))
		{
			UE_LOG(LogTemp, Error, TEXT("[%s] 머티리얼에 'Opacity' 파라미터가 없습니다! 페이드 연출이 작동하지 않습니다."), *GetName());
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("%s: Opacity Parameter Missing!"), *GetName()));
		}
	}

	CurrentRadius = StartRadius;
	StartingRadiusForLerp = StartRadius;

	// 이동 방향 * 속력
	if (IsValid(MovementComponent))
	{
		MovementComponent->Velocity = GetActorForwardVector() * InitialForwardSpeed;
	}
	
	// 히트박스 구
	if (PuddleCollision)
	{
		PuddleCollision->SetSphereRadius(CurrentRadius);
		PuddleCollision->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnPuddleOverlap);
	}

	if (HasAuthority() && GetWorld())
	{
		StartTime = GetWorld()->GetTimeSeconds();
		EndTime = StartTime + BaseDuration;

		// 0.1초 10Hz 마다 콜리전 업데이트 함수 실행 하게 함 이게 Tick보다 훨씬 가볍다한다 - 슝민
		GetWorld()->GetTimerManager().SetTimer(PuddleUpdateTimer, this, &ThisClass::UpdatePuddleState, 0.1f, true);
	}
}

void AGP_WaterPuddle::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (PuddleCollision)
	{
		PuddleCollision->SetSphereRadius(StartRadius);
	}
	if (PuddleDecal)
	{
		PuddleDecal->DecalSize = FVector(128.0f, EndRadius, EndRadius);
	}
}
void AGP_WaterPuddle::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGP_WaterPuddle, EndTime);
}

void AGP_WaterPuddle::UpdatePuddleState()
{
    if (bIsBeingAbsorbed || !GetWorld() || !PuddleCollision) return;
	
    // 이동 제어 
	if (IsValid(MovementComponent))
	{
		if (bIsMovingToDestination)
		{
			FVector CurrentLoc = GetActorLocation();
			FVector DirToTarget = DestinationLoc - CurrentLoc;
			DirToTarget.Z = 0.f;

			float CurrentDistance = DirToTarget.Size2D();
			FVector CurrentVel = MovementComponent->Velocity;
			CurrentVel.Z = 0.f;

			// 목적지 도달하면 상태 해제하고 기본속으로 변경
			if (CurrentDistance <= 50.f || FVector::DotProduct(DirToTarget, CurrentVel) <= 0.f)
			{
				bIsMovingToDestination = false;
				MovementComponent->Velocity = CurrentVel.GetSafeNormal2D() * InitialForwardSpeed;
			}
			else
			{
				// Alpha = 1.0 (멀리 있을때, 출발 직후) -> Alpha = 0.0 (도착 직전)
				float Alpha = PullStartDistance > 0.f ? FMath::Clamp(CurrentDistance / PullStartDistance, 0.0f, 1.0f) : 0.0f;
                
				// 도착지에 가까워질수록 PullMaxSpeed에서 InitialForwardSpeed로 감속
				float LerpSpeed = FMath::Lerp(InitialForwardSpeed, PullMaxSpeed, Alpha);
				
				MovementComponent->Velocity = DirToTarget.GetSafeNormal2D() * LerpSpeed; // 방향 * 보간 속력
			}
		}
	}
    
    // 반복해서 쓰이는 시간 관련 변수들을 한 번만 계산하여 재사용
    const float CurrentTime = GetWorld()->GetTimeSeconds();
    const float TimeSinceStart = CurrentTime - StartTime;
    const float TimeUntilEnd = EndTime - CurrentTime;
    const float TotalTime = EndTime - StartTime;

    // 반지름 보간
    const float Alpha = TotalTime > 0.0f ? FMath::Clamp(TimeUntilEnd / TotalTime, 0.0f, 1.0f) : 0.0f;
    CurrentRadius = FMath::Lerp(EndRadius, StartingRadiusForLerp, Alpha);

    PuddleCollision->SetSphereRadius(CurrentRadius);
    if (PuddleDecal)
    {
        PuddleDecal->DecalSize = FVector(128.0f, CurrentRadius, CurrentRadius);
    }

    // 통합된 페이드 인아웃 제어
    if (DecalDynamicMaterial)
    {
        float CurrentOpacity = 1.0f;

        // 페이드 인 구간
        if (bEnableFadeIn && TimeSinceStart < FadeInDuration)
        {
            CurrentOpacity = FMath::Clamp(TimeSinceStart / FMath::Max(FadeInDuration, 0.001f), 0.0f, 1.0f);
        }
        // 페이드 아웃 구간
        else if (bEnableFadeOut && TimeUntilEnd < FadeOutDuration)
        {
            CurrentOpacity = FMath::Clamp(TimeUntilEnd / FMath::Max(FadeOutDuration, 0.001f), 0.0f, 1.0f);
        }

        DecalDynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), CurrentOpacity);
    }

    // 5. 시간이 다 되면 파괴 처리
    if (CurrentTime >= EndTime)
    {
        bIsBeingAbsorbed = true;
        GetWorld()->GetTimerManager().ClearTimer(PuddleUpdateTimer);
        
        BP_OnFadeOutAndDestroy();
        
        // 투명해진 껍데기 액터가 남지 않도록 SetLifeSpan(1.0f) 대신 즉시 파괴
        Destroy(); 
    }
}

// AI가 마음대로 짠 로직 제거하고 다시 짬 - 슝민
void AGP_WaterPuddle::OnPuddleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !IsValid(OtherActor) || OtherActor == this || bIsBeingAbsorbed || !GetWorld())
	{
		return;
	}

	if (AGP_WaterPuddle* OtherPuddle = Cast<AGP_WaterPuddle>(OtherActor))
	{
		if (OtherPuddle->bIsBeingAbsorbed)
		{
			return;
		}

		if (CurrentRadius > OtherPuddle->CurrentRadius || (CurrentRadius == OtherPuddle->CurrentRadius && this > OtherPuddle))
		{
			// 흡수되는 쪽 타이머 정리
			OtherPuddle->bIsBeingAbsorbed = true;
			OtherPuddle->GetWorld()->GetTimerManager().ClearTimer(OtherPuddle->PuddleUpdateTimer);
			OtherPuddle->PuddleCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);

			// 흡수할 때 시간을 더해줌 : 흡수되는 쪽의 남은 시간의 50퍼 흡수
			const float OtherRemainingTime = FMath::Max(0.0f, OtherPuddle->EndTime - GetWorld()->GetTimeSeconds());

			// 내 현재 시점부터 다시 보간 시작하기 위한 갱신
			StartTime = GetWorld()->GetTimeSeconds();
			StartingRadiusForLerp = FMath::Clamp(CurrentRadius + (OtherPuddle->CurrentRadius * 0.5f), EndRadius, StartRadius);
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
	if (!GetWorld())
	{
		return;
	}

	// 클라이언트에서 EndTime(종료 시간)을 서버로부터 받았을 때 블루프린트로 알려줍니다.
	const float RemainingTime = FMath::Max(0.0f, EndTime - GetWorld()->GetTimeSeconds());
	BP_OnEndTimeUpdated(RemainingTime);
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

void AGP_WaterPuddle::CommandPullTowards(AActor* TargetActor, float PullSpeed)
{
	PullTowards(TargetActor, PullSpeed);
	bIsMovingToDestination = false;
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
	
	PullStartDistance = FVector::Dist2D(GetActorLocation(), DestinationLoc);
	PullMaxSpeed = MoveSpeed;

	// 기존 유도탄 기능이 켜져있을 수 있으니 끄기
	MovementComponent->bIsHomingProjectile = false;
	MovementComponent->MaxSpeed = MoveSpeed;

	// 목표를 향한 방향 벡터 계산 및 속도 적용
	FVector Direction = (DestinationLoc - GetActorLocation()).GetSafeNormal();
	MovementComponent->Velocity = Direction * MoveSpeed;
}
