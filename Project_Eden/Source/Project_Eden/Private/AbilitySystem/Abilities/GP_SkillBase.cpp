#include "AbilitySystem/Abilities/GP_SkillBase.h"
#include "Utils/GP_BlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

UGP_SkillBase::UGP_SkillBase()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGP_SkillBase::PerformAreaAttack()
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar) return;

	// 1. 메커니즘: C++에서 정의된 유틸리티 라이브러리를 사용하여 범위 판정 수행
	TArray<AActor*> HitActors = UGP_BlueprintLibrary::SphereMeleeHitBoxOverlap(
		Avatar,
		AttackRadius,   // 수치: BP에서 설정됨
		ForwardOffset,  // 수치: BP에서 설정됨
		0.0f,
		bDrawDebugs);

	// 2. 이펙트 배달: 찾은 적들에게 데미지 이펙트 적용
	if (HasAuthority(&CurrentActivationInfo) && DamageEffectClass)
	{
		UGP_BlueprintLibrary::ApplyGameplayEffectToActors(
			Avatar,
			HitActors,
			DamageEffectClass,
			GetAbilityLevel());
	}

	// 3. (옵션) 피격 반응 태그 전송 등 공통 로직 처리
}

void UGP_SkillBase::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bInterrupted);
}
