#include "AbilitySystem/Abilities/GP_SkillBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GP_SkillData.h"
#include "Characters/GP_BaseCharacter.h"
#include "GameplayEffect.h"
#include "GameplayTags/GP_Tags.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Player/GP_PlayerState.h"
#include "Utils/GP_BlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

UGP_SkillBase::UGP_SkillBase()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UGP_SkillBase::CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	const UGP_SkillData* SkillData = GetSkillDataFromSpec(Handle, ActorInfo);
	if (!SkillData)
	{
		return Super::CheckCooldown(Handle, ActorInfo, OptionalRelevantTags);
	}

	if (SkillData->CooldownPolicy == EGP_CooldownPolicy::None)
	{
		return true;
	}

	if (SkillData->CooldownPolicy == EGP_CooldownPolicy::Custom)
	{
		return true;
	}

	if (!SkillData->CooldownTag.IsValid())
	{
		return true;
	}

	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC)
	{
		return true;
	}

	if (ASC->HasMatchingGameplayTag(SkillData->CooldownTag))
	{
		if (OptionalRelevantTags)
		{
			OptionalRelevantTags->AddTag(SkillData->CooldownTag);
		}

		return false;
	}

	return true;
}

void UGP_SkillBase::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	const UGP_SkillData* SkillData = GetSkillDataFromSpec(Handle, ActorInfo);
	if (!SkillData)
	{
		Super::ApplyCooldown(Handle, ActorInfo, ActivationInfo);
		return;
	}

	if (SkillData->CooldownPolicy != EGP_CooldownPolicy::Generic)
	{
		return;
	}

	if (!SkillData->CooldownTag.IsValid() || SkillData->CooldownDuration <= 0.f)
	{
		return;
	}

	if (!GenericCooldownEffectClass)
	{
		return;
	}

	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(GenericCooldownEffectClass, GetAbilityLevel(Handle, ActorInfo));
	if (!SpecHandle.IsValid())
	{
		return;
	}

	SpecHandle.Data->DynamicGrantedTags.AddTag(SkillData->CooldownTag);
	const float CooldownMultiplier = GetSkillAugmentCooldownMultiplier(SkillData, ActorInfo);
	const float CooldownDuration = SkillData->CooldownDuration * CooldownMultiplier;
	if (CooldownDuration <= 0.0f)
	{
		return;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Cooldown::Data::Duration, CooldownDuration);
	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
}

UGP_SkillData* UGP_SkillBase::GetSkillDataFromSpec(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const
{
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC)
	{
		return nullptr;
	}

	const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle);
	return Spec ? Cast<UGP_SkillData>(Spec->SourceObject.Get()) : nullptr;
}

FGameplayTag UGP_SkillBase::GetCurrentTechElementTag(const FGameplayAbilityActorInfo* ActorInfo) const
{
	const AActor* OwnerActor = ActorInfo ? ActorInfo->OwnerActor.Get() : nullptr;
	if (const AGP_PlayerState* PlayerState = Cast<AGP_PlayerState>(OwnerActor))
	{
		return PlayerState->GetCurrentTechElementTag();
	}

	const APawn* AvatarPawn = ActorInfo ? Cast<APawn>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (AvatarPawn)
	{
		if (const AGP_PlayerState* PlayerState = AvatarPawn->GetPlayerState<AGP_PlayerState>())
		{
			return PlayerState->GetCurrentTechElementTag();
		}
	}

	return FGameplayTag();
}

float UGP_SkillBase::GetSkillAugmentRadiusMultiplier(const UGP_SkillData* SkillData, const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (!SkillData || !SkillData->SkillIdTag.IsValid())
	{
		return 1.0f;
	}

	const AActor* OwnerActor = ActorInfo ? ActorInfo->OwnerActor.Get() : nullptr;
	if (const AGP_PlayerState* PlayerState = Cast<AGP_PlayerState>(OwnerActor))
	{
		return PlayerState->GetSkillAugmentRadiusMultiplier(SkillData->SkillIdTag);
	}

	const APawn* AvatarPawn = ActorInfo ? Cast<APawn>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (AvatarPawn)
	{
		if (const AGP_PlayerState* PlayerState = AvatarPawn->GetPlayerState<AGP_PlayerState>())
		{
			return PlayerState->GetSkillAugmentRadiusMultiplier(SkillData->SkillIdTag);
		}
	}

	return 1.0f;
}

float UGP_SkillBase::GetSkillAugmentRangeMultiplier(const UGP_SkillData* SkillData, const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (!SkillData || !SkillData->SkillIdTag.IsValid())
	{
		return 1.0f;
	}

	const AActor* OwnerActor = ActorInfo ? ActorInfo->OwnerActor.Get() : nullptr;
	if (const AGP_PlayerState* PlayerState = Cast<AGP_PlayerState>(OwnerActor))
	{
		return PlayerState->GetSkillAugmentRangeMultiplier(SkillData->SkillIdTag);
	}

	const APawn* AvatarPawn = ActorInfo ? Cast<APawn>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (AvatarPawn)
	{
		if (const AGP_PlayerState* PlayerState = AvatarPawn->GetPlayerState<AGP_PlayerState>())
		{
			return PlayerState->GetSkillAugmentRangeMultiplier(SkillData->SkillIdTag);
		}
	}

	return 1.0f;
}

float UGP_SkillBase::GetSkillAugmentCooldownMultiplier(const UGP_SkillData* SkillData, const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (!SkillData || !SkillData->SkillIdTag.IsValid())
	{
		return 1.0f;
	}

	const AActor* OwnerActor = ActorInfo ? ActorInfo->OwnerActor.Get() : nullptr;
	if (const AGP_PlayerState* PlayerState = Cast<AGP_PlayerState>(OwnerActor))
	{
		return PlayerState->GetSkillAugmentCooldownMultiplier(SkillData->SkillIdTag);
	}

	const APawn* AvatarPawn = ActorInfo ? Cast<APawn>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (AvatarPawn)
	{
		if (const AGP_PlayerState* PlayerState = AvatarPawn->GetPlayerState<AGP_PlayerState>())
		{
			return PlayerState->GetSkillAugmentCooldownMultiplier(SkillData->SkillIdTag);
		}
	}

	return 1.0f;
}

int32 UGP_SkillBase::GetSkillAugmentProjectileCountBonus(const UGP_SkillData* SkillData, const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (!SkillData || !SkillData->SkillIdTag.IsValid())
	{
		return 0;
	}

	const AActor* OwnerActor = ActorInfo ? ActorInfo->OwnerActor.Get() : nullptr;
	if (const AGP_PlayerState* PlayerState = Cast<AGP_PlayerState>(OwnerActor))
	{
		return PlayerState->GetSkillAugmentProjectileCountBonus(SkillData->SkillIdTag);
	}

	const APawn* AvatarPawn = ActorInfo ? Cast<APawn>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (AvatarPawn)
	{
		if (const AGP_PlayerState* PlayerState = AvatarPawn->GetPlayerState<AGP_PlayerState>())
		{
			return PlayerState->GetSkillAugmentProjectileCountBonus(SkillData->SkillIdTag);
		}
	}

	return 0;
}

TSubclassOf<AActor> UGP_SkillBase::GetSkillVisualActorClass(const UGP_SkillData* SkillData, TSubclassOf<AActor> FallbackVisualActorClass, FGameplayTag ElementTag) const
{
	if (SkillData && ElementTag.IsValid())
	{
		for (const FGP_ElementVisualActorEntry& Entry : SkillData->ElementVisualActorClasses)
		{
			if (Entry.ElementTag.MatchesTagExact(ElementTag) && Entry.VisualActorClass)
			{
				return Entry.VisualActorClass;
			}
		}
	}

	if (SkillData && SkillData->SkillVisualActorClass)
	{
		return SkillData->SkillVisualActorClass;
	}

	if (FallbackVisualActorClass)
	{
		return FallbackVisualActorClass;
	}

	if (SkillData)
	{
		for (const FGP_ElementVisualActorEntry& Entry : SkillData->ElementVisualActorClasses)
		{
			if (Entry.VisualActorClass)
			{
				return Entry.VisualActorClass;
			}
		}
	}

	return nullptr;
}

TSubclassOf<AActor> UGP_SkillBase::GetSkillSpawnActorClass(const UGP_SkillData* SkillData, TSubclassOf<AActor> FallbackActorClass) const
{
	if (SkillData && SkillData->SpawnActorClass)
	{
		return SkillData->SpawnActorClass;
	}

	return FallbackActorClass;
}

UNiagaraSystem* UGP_SkillBase::GetProjectileVisualSystem(const UGP_SkillData* SkillData, FGameplayTag ElementTag) const
{
	if (!SkillData)
	{
		return nullptr;
	}

	if (ElementTag.IsValid())
	{
		for (const FGP_ElementVisualActorEntry& Entry : SkillData->ElementVisualActorClasses)
		{
			if (Entry.ElementTag.MatchesTagExact(ElementTag))
			{
				return Entry.ProjectileVisualSystem;
			}
		}
	}

	for (const FGP_ElementVisualActorEntry& Entry : SkillData->ElementVisualActorClasses)
	{
		if (Entry.ProjectileVisualSystem)
		{
			return Entry.ProjectileVisualSystem;
		}
	}

	return nullptr;
}

void UGP_SkillBase::SpawnVisualActor(AActor* InstigatorActor, TSubclassOf<AActor> VisualActorClass, const FVector& Location, const FRotator& Rotation, float VisualScale) const
{
	if (!IsValid(InstigatorActor) || !VisualActorClass || !InstigatorActor->GetWorld())
	{
		return;
	}

	if (AGP_BaseCharacter* BaseCharacter = Cast<AGP_BaseCharacter>(InstigatorActor))
	{
		BaseCharacter->ShowSkillVisualActor(VisualActorClass, Location, Rotation, VisualScale);
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = InstigatorActor;
	SpawnParams.Instigator = Cast<APawn>(InstigatorActor);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* VisualActor = InstigatorActor->GetWorld()->SpawnActor<AActor>(
		VisualActorClass,
		Location,
		Rotation,
		SpawnParams
	);

	if (IsValid(VisualActor))
	{
		VisualActor->SetActorScale3D(FVector(FMath::Max(VisualScale, 0.0f)));
	}
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
	TSubclassOf<UGameplayEffect> ResolvedDamageEffectClass = DamageEffectClass;
	if (!ResolvedDamageEffectClass)
	{
		ResolvedDamageEffectClass = LoadClass<UGameplayEffect>(nullptr, TEXT("/Game/GAS_Pattern/AbilitySystem/GameplayEffects/Damage/GE_PrimaryDamage.GE_PrimaryDamage_C"));
	}

	if (HasAuthority(&CurrentActivationInfo) && ResolvedDamageEffectClass)
	{
		UGP_BlueprintLibrary::ApplyGameplayEffectToActors(
			Avatar,
			HitActors,
			ResolvedDamageEffectClass,
			GetAbilityLevel());
	}

	// 3. (옵션) 피격 반응 태그 전송 등 공통 로직 처리
}

void UGP_SkillBase::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bInterrupted);
}
