#include "Characters/GP_BaseCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/GP_AttributeSet.h"
#include "Items/WeaponItemTypes.h"
#include "UI/GP_DamageNumberActor.h"
#include "Animation/PDA_CharacterAnimationSet.h"
#include "Animation/GP_CharacterAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"

AGP_BaseCharacter::AGP_BaseCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	// 대디 서버에서 보이든 안 보이든 애니메이션이 멈추지 않도록 가시성 무관 애니메이션 유지 - 슝민
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	DamageNumberActorClass = AGP_DamageNumberActor::StaticClass();

	OnASCInitialized.AddDynamic(this, &ThisClass::BindAttributeDelegates);
}

void AGP_BaseCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// 데이터 에셋 기반 자동 초기화
	UpdateAnimationSet();
}

void AGP_BaseCharacter::UpdateAnimationSet()
{
	if (!IsValid(AnimationSet)) return;

	// 1. 스켈레탈 메시 업데이트
	if (AnimationSet->CharacterMesh)
	{
		GetMesh()->SetSkeletalMeshAsset(AnimationSet->CharacterMesh);
	}

	// 2. 애니메이션 블루프린트 클래스 교체 (스켈레톤 불일치 해결)
	if (AnimationSet->AnimBlueprintClass)
	{
		GetMesh()->SetAnimInstanceClass(AnimationSet->AnimBlueprintClass);
	}

	// 3. 애니메이션 데이터 주입
	if (UGP_CharacterAnimInstance* AnimInst = Cast<UGP_CharacterAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		AnimInst->SetAnimationSet(AnimationSet);
	}
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

void AGP_BaseCharacter::BindAttributeDelegates(UAbilitySystemComponent* ASC, UAttributeSet* AS)
{
	if (UGP_AttributeSet* GP_AS = Cast<UGP_AttributeSet>(AS))
	{
		// ASC와 AttributeSet이 완벽히 준비된 시점이므로 안전하게 바인딩!
		GP_AS->OnDamageTaken.AddDynamic(this, &ThisClass::HandleDamageTaken);
	}
}

void AGP_BaseCharacter::HandleDamageTaken(AActor* InstigatorActor, AActor* TargetActor, float DamageAmount, FGameplayTag ElementTag)
{
	// 내가 맞은 게 아니면 무시
	if (TargetActor != this) return;

	EWeaponElement ElementToShow = EWeaponElement::Pyros;

	if (ElementTag.MatchesTagExact(GPTags::Damage::Element::Hydro))
	{
		ElementToShow = EWeaponElement::Hydro;
	}
	else if (ElementTag.MatchesTagExact(GPTags::Damage::Element::Volt))
	{
		ElementToShow = EWeaponElement::Volt;
	}
	else if (ElementTag.MatchesTagExact(GPTags::Damage::Element::Aero))
	{
		ElementToShow = EWeaponElement::Aero;
	}
	else if (ElementTag.MatchesTagExact(GPTags::Damage::Element::Lux))
	{
		ElementToShow = EWeaponElement::Lux;
	}
	else if (ElementTag.MatchesTagExact(GPTags::Damage::Element::Chaos))
	{
		ElementToShow = EWeaponElement::Chaos;
	}
	else if (ElementTag.MatchesTagExact(GPTags::Damage::Element::Brute))
	{
		ElementToShow = EWeaponElement::Brute;
	}

	ShowDamageNumber(FMath::RoundToInt(DamageAmount), ElementToShow);
}
