#pragma once

#include "CoreMinimal.h"
#include "GP_AnimationTypes.generated.h"

/**
 * 기존 /Game/Blueprints/Data/E_Gait 자산을 대체하는 C++ 정의.
 * 단일 진실원 유지를 위해 명칭과 순서를 기존 자산과 일치시킵니다.
 */
UENUM(BlueprintType)
enum class E_Gait : uint8
{
	Walk,
	Run,
	Sprint
};

/** 기존 /Game/Blueprints/Data/E_Stance 대체 */
UENUM(BlueprintType)
enum class E_Stance : uint8
{
	Stand,
	Crouch
};

/** 기존 /Game/Blueprints/Data/E_MovementMode 대체 */
UENUM(BlueprintType)
enum class E_MovementMode : uint8
{
	None,
	OnGround,
	InAir,
	Sliding,
	Traversing
};

/** 기존 /Game/Blueprints/Data/E_MovementState 대체 */
UENUM(BlueprintType)
enum class E_MovementState : uint8
{
	None,
	Idle,
	Moving
};
