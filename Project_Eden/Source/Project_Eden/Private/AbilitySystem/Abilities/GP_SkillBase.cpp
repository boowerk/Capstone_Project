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
#include "VFX/GP_NiagaraParameterOverride.h"
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
	if (UGP_SkillData* SpecSkillData = Spec ? Cast<UGP_SkillData>(Spec->SourceObject.Get()) : nullptr)
	{
		return SpecSkillData;
	}

	return DefaultSkillData;
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

bool UGP_SkillBase::HasSkillAugmentInfiniteProjectilePierce(
	const UGP_SkillData* SkillData,
	const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (!SkillData || !SkillData->SkillIdTag.IsValid())
	{
		return false;
	}

	const AActor* OwnerActor = ActorInfo ? ActorInfo->OwnerActor.Get() : nullptr;
	if (const AGP_PlayerState* PlayerState = Cast<AGP_PlayerState>(OwnerActor))
	{
		return PlayerState->HasSkillAugmentInfiniteProjectilePierce(SkillData->SkillIdTag);
	}

	const APawn* AvatarPawn = ActorInfo ? Cast<APawn>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (AvatarPawn)
	{
		if (const AGP_PlayerState* PlayerState = AvatarPawn->GetPlayerState<AGP_PlayerState>())
		{
			return PlayerState->HasSkillAugmentInfiniteProjectilePierce(SkillData->SkillIdTag);
		}
	}

	return false;
}

TSubclassOf<AActor> UGP_SkillBase::GetSkillAugmentImpactVisualActorOverride(const UGP_SkillData* SkillData) const
{
	if (!SkillData || !SkillData->SkillIdTag.IsValid())
	{
		return nullptr;
	}

	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	const AActor* OwnerActor = ActorInfo ? ActorInfo->OwnerActor.Get() : nullptr;
	if (const AGP_PlayerState* GPPlayerState = Cast<AGP_PlayerState>(OwnerActor))
	{
		return GPPlayerState->GetSkillAugmentImpactVisualActorOverride(SkillData->SkillIdTag);
	}

	const APawn* AvatarPawn = ActorInfo ? Cast<APawn>(ActorInfo->AvatarActor.Get()) : nullptr;
	return AvatarPawn && AvatarPawn->GetPlayerState<AGP_PlayerState>()
		? AvatarPawn->GetPlayerState<AGP_PlayerState>()->GetSkillAugmentImpactVisualActorOverride(SkillData->SkillIdTag)
		: nullptr;
}

UNiagaraSystem* UGP_SkillBase::GetSkillAugmentActiveVFXOverride(const UGP_SkillData* SkillData) const
{
	if (!SkillData || !SkillData->SkillIdTag.IsValid())
	{
		return nullptr;
	}

	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	const AActor* OwnerActor = ActorInfo ? ActorInfo->OwnerActor.Get() : nullptr;
	if (const AGP_PlayerState* GPPlayerState = Cast<AGP_PlayerState>(OwnerActor))
	{
		return GPPlayerState->GetSkillAugmentActiveVFXOverride(SkillData->SkillIdTag);
	}

	const APawn* AvatarPawn = ActorInfo ? Cast<APawn>(ActorInfo->AvatarActor.Get()) : nullptr;
	return AvatarPawn && AvatarPawn->GetPlayerState<AGP_PlayerState>()
		? AvatarPawn->GetPlayerState<AGP_PlayerState>()->GetSkillAugmentActiveVFXOverride(SkillData->SkillIdTag)
		: nullptr;
}

TArray<FGP_NiagaraParameterOverride> UGP_SkillBase::GetSkillAugmentNiagaraParameterOverrides(const UGP_SkillData* SkillData) const
{
	if (!SkillData || !SkillData->SkillIdTag.IsValid())
	{
		return {};
	}

	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	const AActor* OwnerActor = ActorInfo ? ActorInfo->OwnerActor.Get() : nullptr;
	if (const AGP_PlayerState* GPPlayerState = Cast<AGP_PlayerState>(OwnerActor))
	{
		return GPPlayerState->GetSkillAugmentNiagaraParameterOverrides(SkillData->SkillIdTag);
	}

	const APawn* AvatarPawn = ActorInfo ? Cast<APawn>(ActorInfo->AvatarActor.Get()) : nullptr;
	return AvatarPawn && AvatarPawn->GetPlayerState<AGP_PlayerState>()
		? AvatarPawn->GetPlayerState<AGP_PlayerState>()->GetSkillAugmentNiagaraParameterOverrides(SkillData->SkillIdTag)
		: TArray<FGP_NiagaraParameterOverride>();
}

FGP_SkillAugmentPeriodicAreaDamage UGP_SkillBase::GetSkillAugmentPeriodicAreaDamage(const UGP_SkillData* SkillData) const
{
	if (!SkillData || !SkillData->SkillIdTag.IsValid())
	{
		return {};
	}

	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	const AActor* OwnerActor = ActorInfo ? ActorInfo->OwnerActor.Get() : nullptr;
	if (const AGP_PlayerState* GPPlayerState = Cast<AGP_PlayerState>(OwnerActor))
	{
		return GPPlayerState->GetSkillAugmentPeriodicAreaDamage(SkillData->SkillIdTag);
	}

	const APawn* AvatarPawn = ActorInfo ? Cast<APawn>(ActorInfo->AvatarActor.Get()) : nullptr;
	return AvatarPawn && AvatarPawn->GetPlayerState<AGP_PlayerState>()
		? AvatarPawn->GetPlayerState<AGP_PlayerState>()->GetSkillAugmentPeriodicAreaDamage(SkillData->SkillIdTag)
		: FGP_SkillAugmentPeriodicAreaDamage();
}

TSubclassOf<AActor> UGP_SkillBase::GetSkillVisualActorClass(const UGP_SkillData* SkillData, TSubclassOf<AActor> FallbackVisualActorClass, FGameplayTag ElementTag, FGameplayTag CueTag) const
{
	if (const TSubclassOf<AActor> AugmentOverride = GetSkillAugmentImpactVisualActorOverride(SkillData))
	{
		return AugmentOverride;
	}

	if (SkillData)
	{
		TSubclassOf<AActor> BestVisualActorClass;
		int32 BestScore = INDEX_NONE;

		for (const FGP_SkillVisualCueEntry& Entry : SkillData->VisualCues)
		{
			if (Entry.VisualType != EGP_SkillVisualType::Actor || !Entry.VisualActorClass)
			{
				continue;
			}

			const bool bCueMatches = !Entry.CueTag.IsValid() || (CueTag.IsValid() && Entry.CueTag.MatchesTagExact(CueTag));
			const bool bElementMatches = !Entry.ElementTag.IsValid() || (ElementTag.IsValid() && Entry.ElementTag.MatchesTagExact(ElementTag));
			if (!bCueMatches || !bElementMatches)
			{
				continue;
			}

			const int32 Score = (Entry.CueTag.IsValid() ? 2 : 0) + (Entry.ElementTag.IsValid() ? 1 : 0);
			if (Score > BestScore)
			{
				BestScore = Score;
				BestVisualActorClass = Entry.VisualActorClass;
			}
		}

		if (BestVisualActorClass)
		{
			return BestVisualActorClass;
		}
	}

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

UNiagaraSystem* UGP_SkillBase::GetSkillNiagaraSystem(const UGP_SkillData* SkillData, FGameplayTag ElementTag, FGameplayTag CueTag) const
{
	if (!SkillData)
	{
		return nullptr;
	}

	UNiagaraSystem* BestNiagaraSystem = nullptr;
	int32 BestScore = INDEX_NONE;

	for (const FGP_SkillVisualCueEntry& Entry : SkillData->VisualCues)
	{
		if (Entry.VisualType != EGP_SkillVisualType::Niagara || !Entry.NiagaraSystem)
		{
			continue;
		}

		const bool bCueMatches = !Entry.CueTag.IsValid() || (CueTag.IsValid() && Entry.CueTag.MatchesTagExact(CueTag));
		const bool bElementMatches = !Entry.ElementTag.IsValid() || (ElementTag.IsValid() && Entry.ElementTag.MatchesTagExact(ElementTag));
		if (!bCueMatches || !bElementMatches)
		{
			continue;
		}

		const int32 Score = (Entry.CueTag.IsValid() ? 2 : 0) + (Entry.ElementTag.IsValid() ? 1 : 0);
		if (Score > BestScore)
		{
			BestScore = Score;
			BestNiagaraSystem = Entry.NiagaraSystem;
		}
	}

	return BestNiagaraSystem;
}

UNiagaraSystem* UGP_SkillBase::GetProjectileVisualSystem(const UGP_SkillData* SkillData, FGameplayTag ElementTag) const
{
	if (!SkillData)
	{
		return nullptr;
	}

	if (UNiagaraSystem* AugmentOverride = GetSkillAugmentActiveVFXOverride(SkillData))
	{
		return AugmentOverride;
	}

	if (UNiagaraSystem* NiagaraSystem = GetSkillNiagaraSystem(SkillData, ElementTag, GPTags::Ability::Skill::Visual::Projectile))
	{
		return NiagaraSystem;
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

void UGP_SkillBase::SpawnVisualActor(
	AActor* InstigatorActor,
	TSubclassOf<AActor> VisualActorClass,
	const FVector& Location,
	const FRotator& Rotation,
	float VisualScale,
	const TArray<FGP_NiagaraParameterOverride>& NiagaraParameterOverrides) const
{
	if (!IsValid(InstigatorActor) || !VisualActorClass || !InstigatorActor->GetWorld())
	{
		return;
	}

	if (AGP_BaseCharacter* BaseCharacter = Cast<AGP_BaseCharacter>(InstigatorActor))
	{
		BaseCharacter->ShowSkillVisualActor(
			VisualActorClass,
			Location,
			Rotation,
			VisualScale,
			NiagaraParameterOverrides);
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
		UGP_SkillData* SkillData = GetSkillDataFromSpec(CurrentSpecHandle, CurrentActorInfo);
		UGP_BlueprintLibrary::ApplyGameplayEffectToActors(
			Avatar,
			HitActors,
			ResolvedDamageEffectClass,
			GetAbilityLevel(),
			SkillData);
	}

	// 3. (옵션) 피격 반응 태그 전송 등 공통 로직 처리
}

void UGP_SkillBase::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bInterrupted);
}
