#include "AbilitySystem/Abilities/Player/Character1/GP_Skill_WaterPuddle.h"
#include "Actors/GP_WaterPuddle.h"
#include "Interfaces/GP_Summonable.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

void UGP_Skill_WaterPuddle::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ACharacter* Avatar = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	
	if (!Avatar || !ASC || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1. 카메라 시선을 기준으로 타겟 좌표(TargetLocation) 구하기
	FVector ViewLocation;
	FRotator ViewRotation;
	Avatar->GetController()->GetPlayerViewPoint(ViewLocation, ViewRotation);

	FVector TraceEnd = ViewLocation + (ViewRotation.Vector() * 10000.f);
	FVector TargetLocation = TraceEnd;
	
	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Avatar);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, ViewLocation, TraceEnd, ECC_Visibility, Params))
	{
		TargetLocation = HitResult.Location;
	}

	// 2. 최대 거리(MaxTargetDistance) 제한 적용
	FVector ToTarget = TargetLocation - Avatar->GetActorLocation();
	ToTarget.Z = 0.f; // 평면 거리만 계산
	
	if (ToTarget.Size() > MaxTargetDistance)
	{
		TargetLocation = Avatar->GetActorLocation() + ToTarget.GetSafeNormal() * MaxTargetDistance;
	}

	// 3. (안전 장치) 허공을 찍었을 경우 바닥 좌표로 내리기
	FVector RayStart = TargetLocation + FVector(0.f, 0.f, 500.f);
	FVector RayEnd = TargetLocation - FVector(0.f, 0.f, 1000.f);
	FHitResult GroundHit;
	if (GetWorld()->LineTraceSingleByChannel(GroundHit, RayStart, RayEnd, ECC_Visibility, Params))
	{
		TargetLocation.Z = GroundHit.Location.Z;
	}

	// =========================================================
	// 4. 모드 분기: 내 몸에 쿨타임 태그가 있는가?
	// =========================================================
	bool bIsOnCooldown = ASC->HasMatchingGameplayTag(CooldownTag);

	if (bIsOnCooldown)
	{
		// 🌊 [모드 A: 패시브] 기존 장판들을 TargetLocation으로 일제히 당겨오기
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGP_WaterPuddle::StaticClass(), FoundActors);

		for (AActor* Actor : FoundActors)
		{
			if (IGP_Summonable* Summonable = Cast<IGP_Summonable>(Actor))
			{
				// 내가 소환한 장판만 조종
				if (Summonable->GetSummonOwner() == Avatar) 
				{
					Summonable->CommandMoveToLocation(TargetLocation, PullSpeed);
				}
			}
		}

		// (필요 시 여기서 PullMontage 재생 Task 추가)
	}
	else
	{
		// 💧 [모드 B: 액티브] 새로운 장판 스폰
		if (HasAuthority(&CurrentActivationInfo) && PuddleClass)
		{
			FTransform SpawnTransform;
			SpawnTransform.SetLocation(TargetLocation);
			SpawnTransform.SetRotation(FQuat::Identity);
			
			// 장판 생성 (Instigator를 나로 설정하여 내 소유임을 명시)
			AGP_WaterPuddle* NewPuddle = GetWorld()->SpawnActorDeferred<AGP_WaterPuddle>(
				PuddleClass, SpawnTransform, Avatar, Avatar, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
				
			if (NewPuddle)
			{
				// 필요하다면 여기서 NewPuddle의 변수 초기화 가능
				UGameplayStatics::FinishSpawningActor(NewPuddle, SpawnTransform);
			}

			// 스폰 성공 시 수동으로 쿨타임 이펙트 적용
			if (ManualCooldownEffectClass)
			{
				FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
				FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(ManualCooldownEffectClass, GetAbilityLevel(), Context);
				ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
		
		// (필요 시 여기서 SpawnMontage 재생 Task 추가)
	}

	// 5. 어빌리티 종료 (몽타주 비동기 처리를 안 하므로 즉시 종료)
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
