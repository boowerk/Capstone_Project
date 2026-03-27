#include "Characters/GP_BaseCharacter.h"
#include "AbilitySystemComponent.h"
#include "Items/WeaponItemTypes.h"
#include "UI/GP_DamageNumberActor.h"


AGP_BaseCharacter::AGP_BaseCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	// 대디 서버에서 보이든 안 보이든 애니메이션이 멈추지 않도록 가시성 무관 애니메이션 유지 - 슝민
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	DamageNumberActorClass = AGP_DamageNumberActor::StaticClass();

}

UAbilitySystemComponent* AGP_BaseCharacter::GetAbilitySystemComponent() const
{
	return nullptr;
}

void AGP_BaseCharacter::GiveStartupAbilities()
{
	if (!IsValid(GetAbilitySystemComponent())) return;
	for (const auto& Ability : StartupAbilities) {
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(Ability);
		GetAbilitySystemComponent()->GiveAbility(AbilitySpec);
	}
}

void AGP_BaseCharacter::InitializeAttributes() const
{
	checkf(IsValid(InitializeAttributesEffect), TEXT("InitializeAttributesEffect not set."));

	FGameplayEffectContextHandle ContextHandle = GetAbilitySystemComponent()->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = GetAbilitySystemComponent()->MakeOutgoingSpec(InitializeAttributesEffect, 1.f, ContextHandle);
	GetAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

void AGP_BaseCharacter::ShowDamageNumber(int32 DamageAmount, EWeaponElement Element)
{
	if (HasAuthority())
	{
		// 서버가 타격 결과를 확정하고 모든 클라이언트에 같은 숫자를 뿌린다.
		MulticastShowDamageNumber(DamageAmount, Element);
		return;
	}

	SpawnDamageNumberActor(DamageAmount, Element);
}

void AGP_BaseCharacter::MulticastShowDamageNumber_Implementation(int32 DamageAmount, EWeaponElement Element)
{
	SpawnDamageNumberActor(DamageAmount, Element);
}

void AGP_BaseCharacter::SpawnDamageNumberActor(int32 DamageAmount, EWeaponElement Element)
{
	if (!IsValid(GetWorld()) || !IsValid(DamageNumberActorClass))
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AGP_DamageNumberActor* DamageNumberActor = GetWorld()->SpawnActor<AGP_DamageNumberActor>(
		DamageNumberActorClass,
		GetActorLocation(),
		FRotator::ZeroRotator,
		SpawnParams);

	if (!IsValid(DamageNumberActor))
	{
		return;
	}

	// 적 위치 기준으로 월드 데미지 숫자 액터를 즉시 초기화한다.
	DamageNumberActor->InitializeDamageNumber(DamageAmount, Element);
}
