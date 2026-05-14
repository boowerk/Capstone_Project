#pragma once

#include "CoreMinimal.h"
#include "GP_AnimationTypes.generated.h"

UENUM(BlueprintType)
enum class E_Gait : uint8
{
	Walking,
	Running,
	Sprinting
};

UENUM(BlueprintType)
enum class E_Stance : uint8
{
	Standing,
	Crouching
};

UENUM(BlueprintType)
enum class E_MovementMode : uint8
{
	None,
	Grounded,
	Falling,
	Swimming,
	Flying
};

UENUM(BlueprintType)
enum class E_MovementState : uint8
{
	None,
	Idle,
	Move,
	Stop,
	Pivot,
	Jump
};
