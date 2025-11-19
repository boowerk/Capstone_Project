#pragma once

#include "CoreMinimal.h"
#include "Characters/GP_BaseCharacter.h"
#include "AbilitySystemInterface.h"

#include "GP_PlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;

UCLASS()
class PROJECT_EDEN_API AGP_PlayerCharacter : public AGP_BaseCharacter
{
	GENERATED_BODY()

public:
	AGP_PlayerCharacter();
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;


};
