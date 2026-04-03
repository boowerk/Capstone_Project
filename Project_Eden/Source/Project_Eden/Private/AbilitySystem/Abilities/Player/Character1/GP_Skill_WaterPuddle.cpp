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
    
    // 유효성 및 어빌리티 커밋 확인
    if (!Avatar || !ASC || !CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
       EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
       return;
    }

    // 카메라 시선쪽 타겟 위치 계산
    FVector ViewLoc;
    FRotator ViewRot;
    Avatar->GetController()->GetPlayerViewPoint(ViewLoc, ViewRot);

    FVector TargetLoc = ViewLoc + (ViewRot.Vector() * 10000.f);
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(PuddleTrace), false, Avatar);

    if (GetWorld()->LineTraceSingleByChannel(Hit, ViewLoc, TargetLoc, ECC_Visibility, Params))
    {
       TargetLoc = Hit.Location;
    }

    // 최대 거리 제한: 루트 연산에서 SizeSquared 사용 최적화
    FVector ToTarget = TargetLoc - Avatar->GetActorLocation();
    ToTarget.Z = 0.f; 
    
    if (ToTarget.SizeSquared() > (MaxTargetDistance * MaxTargetDistance))
    {
       TargetLoc = Avatar->GetActorLocation() + ToTarget.GetSafeNormal() * MaxTargetDistance;
    }

    // 허공 방지 바닥 스냅: 동일한 Hit, Params 변수 재사용
    if (GetWorld()->LineTraceSingleByChannel(Hit, TargetLoc + FVector(0.f, 0.f, 500.f), TargetLoc - FVector(0.f, 0.f, 1000.f), ECC_Visibility, Params))
    {
       TargetLoc.Z = Hit.Location.Z;
    }
    
    // 쿨타임 여부에 따른 모드 분기
    if (ASC->HasMatchingGameplayTag(CooldownTag))
    {
       // [기존 장판 당겨오기: 패시브] 
       TArray<AActor*> FoundActors;
       UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGP_WaterPuddle::StaticClass(), FoundActors);

       for (AActor* Actor : FoundActors)
       {
          if (IGP_Summonable* Summonable = Cast<IGP_Summonable>(Actor))
          {
             if (Summonable->GetSummonOwner() == Avatar) 
             {
                Summonable->CommandMoveToLocation(TargetLoc, PullSpeed);
             }
          }
       }
    }
    else if (HasAuthority(&CurrentActivationInfo) && PuddleClass)
    {
       // [새로운 장판 스폰: 액티브] 
       FTransform SpawnTM(FRotator::ZeroRotator, TargetLoc);
       
       if (AGP_WaterPuddle* NewPuddle = GetWorld()->SpawnActorDeferred<AGP_WaterPuddle>(PuddleClass, SpawnTM, Avatar, Avatar, ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
       {
          UGameplayStatics::FinishSpawningActor(NewPuddle, SpawnTM);
          // 스폰 직후 초기 이동 방향 및 속도 적용
          NewPuddle->InitializeMovement(Avatar);
       }

       // 수동 쿨타임 이펙트 적용
       if (ManualCooldownEffectClass)
       {
          FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(ManualCooldownEffectClass, GetAbilityLevel(), ASC->MakeEffectContext());
          if (SpecHandle.IsValid()) ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
       }
    }

    // 어빌리티 즉시 종료
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
