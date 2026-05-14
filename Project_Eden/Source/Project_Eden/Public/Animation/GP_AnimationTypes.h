#pragma once

#include "CoreMinimal.h"
#include "GP_AnimationTypes.generated.h"

/**
 * [마이그레이션 단계 2] 기존 블루프린트 에넘 자산을 C++로 승격(Migration)합니다.
 * 이 정의는 기존 /Game/Blueprints/Data/E_... 자산의 실제 구조와 완벽히 일치해야 합니다.
 */

UENUM(BlueprintType)
enum class E_Gait : uint8
{
	Walk,
	Run,
	Sprint
};

UENUM(BlueprintType)
enum class E_Stance : uint8
{
	Stand,
	Crouch
};

UENUM(BlueprintType)
enum class E_MovementMode : uint8
{
	None,
	OnGround,
	InAir,
	Sliding,
	Traversing
};

UENUM(BlueprintType)
enum class E_MovementState : uint8
{
	None,
	Idle,
	Moving
};
