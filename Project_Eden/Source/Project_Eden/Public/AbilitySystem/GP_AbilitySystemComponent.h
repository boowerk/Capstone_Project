#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"

#include "GP_AbilitySystemComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECT_EDEN_API UGP_AbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()


protected:
	virtual void BeginPlay() override;

public:
	UGP_AbilitySystemComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
};
