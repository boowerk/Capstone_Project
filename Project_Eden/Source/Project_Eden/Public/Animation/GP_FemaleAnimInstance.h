#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"

#include "GP_FemaleAnimInstance.generated.h"

class ACharacter;
class AGP_PlayerCharacter;
class UAnimSequenceBase;
class UBlendSpace;

UCLASS(Blueprintable)
class PROJECT_EDEN_API UGP_FemaleAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UBlendSpace> LocomotionBlendSpaceAsset = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimSequenceBase> JumpLoopAnimationAsset = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float GroundSpeed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsFalling = false;

private:
	void RefreshAnimationAssets();

	TWeakObjectPtr<ACharacter> CachedCharacter;
	TWeakObjectPtr<AGP_PlayerCharacter> CachedPlayerCharacter;
};
