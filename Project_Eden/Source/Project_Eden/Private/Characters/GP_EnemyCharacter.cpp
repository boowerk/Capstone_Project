#include "Characters/GP_EnemyCharacter.h"

#include "AIController.h"
#include "AbilitySystem/GP_AbilitySystemComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "AI/PlayerBehaviorTreeBuilder.h"
#include "AbilitySystem/GP_AttributeSet.h"


AGP_EnemyCharacter::AGP_EnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	AbilitySystemComponent = CreateDefaultSubobject<UGP_AbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	
	AttributeSet = CreateDefaultSubobject<UGP_AttributeSet>("AttributeSet");

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
	
	// 비헤이비어 트리 추가
	InitializeRuntimeBehaviorTree();
	BehaviorAnchorLocation = GetActorTransform().TransformPosition(BehaviorAnchorOffset); // HasAuthority 단독으로 이동하면, 어빌리티의 구조를 침범하므로 제거 예정
}

void AGP_EnemyCharacter::InitializeRuntimeBehaviorTree()
{
	SpawnDefaultController();
	AAIController* AIController = Cast<AAIController>(GetController());
	if (!IsValid(AIController))
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyAI] Failed to run Behavior Tree: AIController is not valid for %s"), *GetName());
		return;
	}

	BehaviorTreeBuilder = NewObject<UPlayerBehaviorTreeBuilder>(this);
	if (!BehaviorTreeBuilder)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyAI] Failed to create behavior tree builder for %s"), *GetName());
		return;
	}

	RuntimeBehaviorTree = BehaviorTreeBuilder->BuildBehaviorTreeFromJson(BehaviorEvaluationJson);
	if (!IsValid(RuntimeBehaviorTree))
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyAI] Failed to build runtime behavior tree for %s. Json: %s"), *GetName(), *BehaviorEvaluationJson);
		return;
	}

	const bool bIsRunning = AIController->RunBehaviorTree(RuntimeBehaviorTree);
	if (!bIsRunning)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyAI] RunBehaviorTree failed for %s"), *GetName());
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[EnemyAI] Runtime behavior tree applied to %s"), *GetName());
}
