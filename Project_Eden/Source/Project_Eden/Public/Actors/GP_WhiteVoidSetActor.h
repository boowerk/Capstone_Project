#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP_WhiteVoidSetActor.generated.h"

class UGP_WhiteVoidSetComponent;

UCLASS()
class PROJECT_EDEN_API AGP_WhiteVoidSetActor : public AActor
{
	GENERATED_BODY()

public:
	AGP_WhiteVoidSetActor();

	UFUNCTION(BlueprintCallable, Category = "White Void")
	void RebuildWhiteVoidSet();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "White Void")
	TObjectPtr<UGP_WhiteVoidSetComponent> WhiteVoidSetComponent;
};
