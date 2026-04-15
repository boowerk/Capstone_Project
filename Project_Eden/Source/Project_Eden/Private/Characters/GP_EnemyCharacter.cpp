#include "Characters/GP_EnemyCharacter.h"

#include "AI/Controllers/EnemyAIController.h"
#include "AbilitySystem/GP_AbilitySystemComponent.h"
#include "AbilitySystem/GP_AttributeSet.h"

AGP_EnemyCharacter::AGP_EnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	AbilitySystemComponent = CreateDefaultSubobject<UGP_AbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<UGP_AttributeSet>(TEXT("AttributeSet"));

	// 적은 배치/스폰 시 공용 AIController를 자동 점유해 BT/Blackboard 초기화를 컨트롤러에 위임한다.
	AIControllerClass = AEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

UAttributeSet* AGP_EnemyCharacter::GetAttributeSet() const
{
	return AttributeSet;
}

UAbilitySystemComponent* AGP_EnemyCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

FVector AGP_EnemyCharacter::GetBehaviorAnchorLocation() const
{
	return BehaviorAnchorLocation.IsNearlyZero() ? GetActorLocation() : BehaviorAnchorLocation;
}

FText AGP_EnemyCharacter::GetBossDisplayName() const
{
	if (!BossDisplayName.IsEmpty())
	{
		return BossDisplayName;
	}

	return FText::FromString(GetName());
}

void AGP_EnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (!IsValid(GetAbilitySystemComponent())) return;

	GetAbilitySystemComponent()->InitAbilityActorInfo(this, this);
	OnASCInitialized.Broadcast(GetAbilitySystemComponent(), GetAttributeSet());

	if (!HasAuthority()) return;

	GiveStartupAbilities();
	InitializeAttributes();

	// 기준 위치는 캐릭터가 저장하고, 실제 Blackboard/Behavior Tree 시작은 AEnemyAIController::OnPossess에서 담당한다.
	BehaviorAnchorLocation = GetActorTransform().TransformPosition(BehaviorAnchorOffset);
}
