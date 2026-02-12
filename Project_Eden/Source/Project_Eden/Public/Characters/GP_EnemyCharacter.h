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
	//virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//virtual UAttributeSet* GetAttributeSet() const override;

	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crash|AI")
	//float AGPeptanceRadius{ 500.f };

	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crash|AI")
	//float MinAttackDelay{ .1f };

	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crash|AI")
	//float MaxAttackDelay{ .5f };

	//UFUNCTION(BlueprintImplementableEvent)
	//float GetTimelineLength();

	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated)
	//bool bIsBeingLaunched{ false };

	//void StopMovementUntilLanded();
protected:
	virtual void BeginPlay() override;

	/** C++에서 동적으로 생성할 행동트리용 평가 JSON */
	UPROPERTY(EditAnywhere, Category = "AI|BehaviorTree")
	FString BehaviorEvaluationJson = TEXT("{\"aggression\":0.7,\"exploration\":0.3,\"survival\":0.5,\"support\":0.2}");

	//virtual void HandleDeath() override;
private:
	void InitializeRuntimeBehaviorTree();

	//UFUNCTION()
	//void EnableMovementOnLanded(const FHitResult& Hit);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	//UPROPERTY()
	//TObjectPtr<UAttributeSet> AttributeSet;

	UPROPERTY(Transient)
	TObjectPtr<UPlayerBehaviorTreeBuilder> BehaviorTreeBuilder;

	UPROPERTY(Transient)
	TObjectPtr<UBehaviorTree> RuntimeBehaviorTree;
};