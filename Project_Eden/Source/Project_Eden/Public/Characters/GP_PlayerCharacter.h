#pragma once

#include "CoreMinimal.h"
#include "Characters/GP_BaseCharacter.h"
#include "GP_PlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;

UCLASS()
class PROJECT_EDEN_API AGP_PlayerCharacter : public AGP_BaseCharacter
{
	GENERATED_BODY()

public:
	AGP_PlayerCharacter();

private:
	UPROPERTY(VisibleAnywhere, Category = "Camera") // Ä«¸Þ¶ó ¾Ï - ½¹¹Î
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera") // ÆÈ·Î¿ì Ä«¸Þ¶ó - ½¹¹Î
	TObjectPtr<UCameraComponent> FollowCamera;


};