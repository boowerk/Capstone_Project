#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GP_MainMenuGameMode.generated.h"

class UGP_MainMenuWidget;

UCLASS()
class PROJECT_EDEN_API AGP_MainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UGP_MainMenuWidget> MainMenuWidgetClass;

private:
	UPROPERTY()
	TObjectPtr<UGP_MainMenuWidget> MainMenuWidget;
};
