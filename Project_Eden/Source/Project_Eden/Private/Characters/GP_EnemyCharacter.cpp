#include "Characters/GP_EnemyCharacter.h"

#include "AbilitySystemComponent.h"
//#include "AbilitySystemBlueprintLibrary.h"
//#include "AIController.h"
//#include "AbilitySystem/GP_AbilitySystemComponent.h"
//#include "AbilitySystem/GP_AttributeSet.h"
//#include "GameplayTags/GPTags.h"
//#include "Net/UnrealNetwork.h"


AGP_EnemyCharacter::AGP_EnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	//AttributeSet = CreateDefaultSubobject<UGP_AttributeSet>("AttributeSet");
}

//void AGP_EnemyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
//{
//	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
//
//	DOREPLIFETIME(ThisClass, bIsBeingLaunched);
//}

UAbilitySystemComponent* AGP_EnemyCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

//UAttributeSet* AGP_EnemyCharacter::GetAttributeSet() const
//{
//	return AttributeSet;
//}

//void AGP_EnemyCharacter::StopMovementUntilLanded()
//{
//	bIsBeingLaunched = true;
//	AAIController* AIController = GetController<AAIController>();
//	if (!IsValid(AIController)) return;
//	AIController->StopMovement();
//	if (!LandedDelegate.IsAlreadyBound(this, &ThisClass::EnableMovementOnLanded))
//	{
//		LandedDelegate.AddDynamic(this, &ThisClass::EnableMovementOnLanded);
//	}
//}

//void AGP_EnemyCharacter::EnableMovementOnLanded(const FHitResult& Hit)
//{
//	bIsBeingLaunched = false;
//	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, GPTags::Events::Enemy::EndAttack, FGameplayEventData());
//	LandedDelegate.RemoveAll(this);
//}

void AGP_EnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (!IsValid(GetAbilitySystemComponent())) return;

	GetAbilitySystemComponent()->InitAbilityActorInfo(this, this);
//	OnASCInitialized.Broadcast(GetAbilitySystemComponent(), GetAttributeSet());
//
	if (!HasAuthority()) return;

	GiveStartupAbilities();
//	InitializeAttributes();
//
//	UGP_AttributeSet* GP_AttributeSet = Cast<UGP_AttributeSet>(GetAttributeSet());
//	if (!IsValid(GP_AttributeSet)) return;
//
//	GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(GP_AttributeSet->GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);
}

//void AGP_EnemyCharacter::HandleDeath()
//{
//	Super::HandleDeath();
//
//	AAIController* AIController = GetController<AAIController>();
//	if (!IsValid(AIController)) return;
//	AIController->StopMovement();
//}
