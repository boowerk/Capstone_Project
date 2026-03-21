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

	if (!HasAuthority()) return;

	GiveStartupAbilities();
	
	// 諛깆???異붽? 
	InitializeRuntimeBehaviorTree();
	BehaviorAnchorLocation = GetActorTransform().TransformPosition(BehaviorAnchorOffset); // HasAuthority ?먮룆 ?꾨줈 ?대룞, ?대퉴由ы떚??援ъ“瑜?移⑤쾾?섎?濡??쒓굅?덉젙
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
