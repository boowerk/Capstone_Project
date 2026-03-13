#pragma once

#include "CoreMinimal.h"
#include "Characters/GP_BaseCharacter.h"
#include "GP_EnemyCharacter.generated.h"

class UAbilitySystemComponent;
class UAttributeSet;
class UPlayerBehaviorTreeBuilder;
class UBehaviorTree;

UCLASS()
class PROJECT_EDEN_API AGP_EnemyCharacter : public AGP_BaseCharacter
{
	GENERATED_BODY()

public:
	AGP_EnemyCharacter();
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	FVector GetBehaviorAnchorLocation() const;//

protected:
	virtual void BeginPlay() override;

	//뭔지모를 똥트리
	UPROPERTY(EditAnywhere, Category = "AI|BehaviorTree")
	FString BehaviorEvaluationJson = TEXT("{\"aggression\":0.7,\"exploration\":0.3,\"survival\":0.5,\"support\":0.2}");
	
	UPROPERTY(EditInstanceOnly, Category = "AI|BehaviorTree", meta = (MakeEditWidget = "true"))
	FVector BehaviorAnchorOffset = FVector::ZeroVector;
private:
	

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
	//백지훈코드 - 어빌리티의 구조를 침범하므로 제거예정
	void InitializeRuntimeBehaviorTree();
	UPROPERTY(Transient)
	TObjectPtr<UPlayerBehaviorTreeBuilder> BehaviorTreeBuilder;

	UPROPERTY(Transient)
	TObjectPtr<UBehaviorTree> RuntimeBehaviorTree;

	FVector BehaviorAnchorLocation = FVector::ZeroVector;
};