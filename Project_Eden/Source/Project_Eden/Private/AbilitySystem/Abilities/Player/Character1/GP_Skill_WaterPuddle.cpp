#include "AbilitySystem/Abilities/Player/Character1/GP_Skill_WaterPuddle.h"
#include "AbilitySystem/Abilities/GP_SkillBase.h"
#include "Actors/GP_WaterPuddle.h"
#include "Interfaces/GP_Summonable.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameplayTags/GP_Tags.h"

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

    // 1. 현재 어떤 슬롯에서 실행되었는지 확인 (DynamicSpecSourceTags 이용)
    FGameplayTag CurrentSlotTag;
    FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec();
    if (Spec)
    {
        const FGameplayTagContainer& DynamicTags = Spec->GetDynamicSpecSourceTags();
        for (const FGameplayTag& Tag : DynamicTags)
        {
            if (Tag.MatchesTag(GPTags::Ability::Skill::SkillRoot))
            {
                CurrentSlotTag = Tag;
                break;
            }
        }
    }

    // 2. 현재 내 슬롯에 해당하는 웅덩이가 이미 있는지 월드에서 직접 검색 (가장 확실한 소스)
    AGP_WaterPuddle* ExistingPuddle = nullptr;
    {
        TArray<AActor*> FoundActors;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGP_WaterPuddle::StaticClass(), FoundActors);
        for (AActor* Actor : FoundActors)
        {
            AGP_WaterPuddle* Puddle = Cast<AGP_WaterPuddle>(Actor);
            if (Puddle && Puddle->GetSummonOwner() == Avatar)
            {
                // 공유 모드거나, 내 슬롯 태그가 포함된 웅덩이인 경우
                if (bShareAcrossSlots || Puddle->GetAssignedSlotTags().HasTagExact(CurrentSlotTag))
                {
                    ExistingPuddle = Puddle;
                    break;
                }
            }
        }
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

    FVector ToTarget = TargetLoc - Avatar->GetActorLocation();
    ToTarget.Z = 0.f; 
    if (ToTarget.SizeSquared() > (MaxTargetDistance * MaxTargetDistance))
    {
       TargetLoc = Avatar->GetActorLocation() + ToTarget.GetSafeNormal() * MaxTargetDistance;
    }

    if (GetWorld()->LineTraceSingleByChannel(Hit, TargetLoc + FVector(0.f, 0.f, 500.f), TargetLoc - FVector(0.f, 0.f, 1000.f), ECC_Visibility, Params))
    {
       TargetLoc.Z = Hit.Location.Z;
    }
    
    // 3. 웅덩이 존재 여부에 따른 모드 분기
    if (ExistingPuddle)
    {
        // [기존 장판 당겨오기] - 서버/클라이언트 공통 실행 (예측 가능)
        ExistingPuddle->CommandMoveToLocation(TargetLoc, PullSpeed);
    }
    else if (HasAuthority(&CurrentActivationInfo) && PuddleClass)
    {
        // [새로운 장판 스폰] - 서버 전용
        FTransform SpawnTM(FRotator::ZeroRotator, TargetLoc);
        if (AGP_WaterPuddle* NewPuddle = GetWorld()->SpawnActorDeferred<AGP_WaterPuddle>(PuddleClass, SpawnTM, Avatar, Avatar, ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
        {
            NewPuddle->SetAssignedSlotTag(CurrentSlotTag);
            UGameplayStatics::FinishSpawningActor(NewPuddle, SpawnTM);
            NewPuddle->InitializeMovement(Avatar);
        }

        // 쿨타임 이펙트 적용 (필요 시)
        if (ManualCooldownEffectClass)
        {
            FGameplayEffectSpecHandle CooldownSpecHandle = ASC->MakeOutgoingSpec(ManualCooldownEffectClass, GetAbilityLevel(), ASC->MakeEffectContext());
            if (CooldownSpecHandle.IsValid())
            {
                ASC->ApplyGameplayEffectSpecToSelf(*CooldownSpecHandle.Data.Get());
            }
        }
    }

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
