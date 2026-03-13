#include "Characters/GP_EnemyCharacter.h"

#include "AIController.h"
#include "AbilitySystem/GP_AbilitySystemComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "AI/PlayerBehaviorTreeBuilder.h"
//#include "AbilitySystemBlueprintLibrary.h"
//#include "AbilitySystem/GP_AbilitySystemComponent.h"
//#include "AbilitySystem/GP_AttributeSet.h"
//#include "GameplayTags/GPTags.h"
//#include "Net/UnrealNetwork.h"


AGP_EnemyCharacter::AGP_EnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	AbilitySystemComponent = CreateDefaultSubobject<UGP_AbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

}



UAbilitySystemComponent* AGP_EnemyCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

FVector AGP_EnemyCharacter::GetBehaviorAnchorLocation() const
{
	return BehaviorAnchorLocation.IsNearlyZero() ? GetActorLocation() : BehaviorAnchorLocation;
}


void AGP_EnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (!IsValid(GetAbilitySystemComponent())) return;

	GetAbilitySystemComponent()->InitAbilityActorInfo(this, this);

	if (!HasAuthority()) return;

	GiveStartupAbilities();
	
	// 백지훈 추가 
	InitializeRuntimeBehaviorTree();
	BehaviorAnchorLocation = GetActorTransform().TransformPosition(BehaviorAnchorOffset); // HasAuthority 판독 후로 이동, 어빌리티의 구조를 침범하므로 제거예정
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