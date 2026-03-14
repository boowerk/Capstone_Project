#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GP_GameplayAbility.generated.h"

UCLASS()
class PROJECT_EDEN_API UGP_GameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
public:
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Debug")
	bool bDrawDebugs = false;
};
