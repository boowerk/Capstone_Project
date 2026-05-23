#pragma once

#include "Animation/GP_CharacterAnimInstance.h"
#include "GP_BossAnimInstance.generated.h"

// Boss-specific animation instance keeps Sans on the player-style animation data path while leaving room for pattern states.
UCLASS()
class PROJECT_EDEN_API UGP_BossAnimInstance : public UGP_CharacterAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
};
