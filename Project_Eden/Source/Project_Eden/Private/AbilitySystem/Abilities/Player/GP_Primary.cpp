#include "AbilitySystem/Abilities/Player/GP_Primary.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/GP_AttributeSet.h"
#include "Engine/OverlapResult.h"
#include "GameplayTags/GP_Tags.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"

// 이 부분은 BP에도 관상용으로 만들어뒀으니 블루프린트 코드로 만들고싶으면 보셈 - 슝민
void UGP_Primary::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// Super::ActivateAbility가 호출되어야 블루프린트의 "Event ActivateAbility" 노드도 실행됩니다.
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	// Sequence 1: Gameplay Event 대기 Task 생성 
	if (AttackEventTag.IsValid())
	{
		UAbilityTask_WaitGameplayEvent* WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AttackEventTag, nullptr, false, true);
		if (WaitEventTask)
		{
			// 이벤트가 들어오면 OnAttackEventReceived()
			WaitEventTask->EventReceived.AddDynamic(this, &ThisClass::OnAttackEventReceived);
			WaitEventTask->ReadyForActivation();
		}
	}
	
	// Sequence 0: 몽타주 Task 생성
	if (AttackMontage) // 몽타주가 할당시 CPP 내부에서 자동 재생하고 종료 처리
	{
		UAbilityTask_PlayMontageAndWait* PlayMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, AttackMontage, 1.0f);
		if (PlayMontageTask)
		{
			// 애니메이션이 끝나거나 취소되면 OnMontageCompleted()
			PlayMontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
			PlayMontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageCompleted);
			PlayMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageCompleted);
			PlayMontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCompleted);
			
			PlayMontageTask->ReadyForActivation();
		}
	}
	else // 몽타주가 비어있음 블루프린트 몽타주로
	{
		if (bDrawDebugs)
		{
			UE_LOG(LogTemp, Warning, TEXT("Primary Attack: C++ Montage is NULL. Waiting for Blueprint to handle Montage."));
		}
	}
}

void UGP_Primary::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGP_Primary::OnAttackEventReceived(FGameplayEventData Payload)
{
	// 블루프린트의 Wait Gameplay Event -> Hitbox Overlap Test -> For Each Loop 역할

	// 1. 히트박스 판정
	TArray<AActor*> HitActors = HitboxOverlapTest();
	
	// 2. 피격 리액션 이벤트 전송
	SendHitReactEventToActors(HitActors);

	// 3. 서버에서만 데미지 이펙트 적용
	if (HasAuthority(&CurrentActivationInfo) && DamageEffectClass)
	{
		for (AActor* HitActor : HitActors)
		{
			UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
			if (TargetASC)
			{
				FGameplayEffectContextHandle ContextHandle = GetAbilitySystemComponentFromActorInfo()->MakeEffectContext();
				// 이펙트 생성 시 인스게이터(공격자) 정보 등이 Context에 담깁니다.
				FGameplayEffectSpecHandle SpecHandle = GetAbilitySystemComponentFromActorInfo()->MakeOutgoingSpec(DamageEffectClass, GetAbilityLevel(), ContextHandle);
				
				// 피격자(TargetASC)에게 데미지 이펙트 적용
				GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
			}
		}
	}
}

TArray<AActor*> UGP_Primary::HitboxOverlapTest()
{
	TArray<AActor*> ActorToIgnore;
	ActorToIgnore.Add(GetAvatarActorFromActorInfo());

	FCollisionQueryParams CollisionQueryParams;
	CollisionQueryParams.AddIgnoredActors(ActorToIgnore);

	FCollisionResponseParams CollisionResponseParams;
	CollisionResponseParams.CollisionResponse.SetAllChannels(ECR_Ignore);
	CollisionResponseParams.CollisionResponse.SetResponse(ECC_Pawn, ECR_Block);

	TArray<FOverlapResult> OverlapResults;
	FCollisionShape CollisionShapeSphere = FCollisionShape::MakeSphere(HitBoxRadius);

	const FVector Forward = GetAvatarActorFromActorInfo()->GetActorForwardVector() * HitBoxForwardOffset;
	const FVector HitBoxLocation = GetAvatarActorFromActorInfo()->GetActorLocation() + Forward + FVector(
		0.f, 0.f, HitBoxElevationOffset);

	GetWorld()->OverlapMultiByChannel(OverlapResults, HitBoxLocation, FQuat::Identity,
	                                  ECC_Visibility, CollisionShapeSphere, CollisionQueryParams,
	                                  CollisionResponseParams);

	TArray<AActor*> ActorsHit;
	for (const FOverlapResult& Result : OverlapResults)
	{
		if (!IsValid(Result.GetActor())) continue;
		ActorsHit.AddUnique(Result.GetActor());
	}

	if (bDrawDebugs)
	{
		DrawDebugsHitBoxOverlap(OverlapResults, HitBoxLocation);
	}

	return ActorsHit;
}

void UGP_Primary::SendHitReactEventToActors(const TArray<AActor*>& ActorsHit)
{
	for (AActor* HitActor : ActorsHit)
	{
		FGameplayEventData Payload;
		Payload.Instigator = GetAvatarActorFromActorInfo();
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(HitActor, GPTags::Events::Enemy::HitReact, Payload);
	}
}

void UGP_Primary::DrawDebugsHitBoxOverlap(const TArray<FOverlapResult>& OverlapResults,
                                          const FVector& HitBoxLocation) const
{
	DrawDebugSphere(GetWorld(), HitBoxLocation, HitBoxRadius, 16, FColor::Red, false, 3.f);
	for (const FOverlapResult& Result : OverlapResults)
	{
		FVector DebugLocation = Result.GetActor()->GetActorLocation();
		DebugLocation.Z += 100.f;
		DrawDebugSphere(GetWorld(), DebugLocation, 30.f, 10, FColor::Green, false, 3.f);
	}
}
