#pragma once

#include "CoreMinimal.h"
#include "Characters/GP_BaseCharacter.h"
#include "GP_EnemyCharacter.generated.h"

class UAbilitySystemComponent;
class UAttributeSet;

UCLASS()
class PROJECT_EDEN_API AGP_EnemyCharacter : public AGP_BaseCharacter
{
	GENERATED_BODY()

public:
	AGP_EnemyCharacter();
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UAttributeSet* GetAttributeSet() const override;

	UFUNCTION(BlueprintPure, Category = "Boss")
	bool IsBossEnemy() const { return bIsBossEnemy; }

	UFUNCTION(BlueprintPure, Category = "Boss")
	FText GetBossDisplayName() const;

	UFUNCTION(BlueprintPure, Category = "AI")
	FVector GetBehaviorAnchorLocation() const;

protected:
	virtual void BeginPlay() override;
	
	// 향후 EQS나 복귀 로직에서 사용할 기준 위치를 월드에 배치할 수 있도록 유지한다.
	UPROPERTY(EditInstanceOnly, Category = "AI", meta = (MakeEditWidget = "true"))
	FVector BehaviorAnchorOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss")
	bool bIsBossEnemy = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss", meta = (EditCondition = "bIsBossEnemy"))
	FText BossDisplayName;
private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY()
	TObjectPtr<UAttributeSet> AttributeSet;

	FVector BehaviorAnchorLocation = FVector::ZeroVector;
};
