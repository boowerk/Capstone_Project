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

/** 실험적 스테이트 머신을 위한 실제 사용 상태 Enum */
UENUM(BlueprintType)
enum class E_ExperimentalStateMachineState : uint8
{
	Idle_Loop,
	Transition_to_Idle_Loop,
	Locomotion_Loop,
	Transition_to_Locomotion_Loop,
	In_Air_Loop,
	Transition_to_In_Air_Loop,
	Idle_Break,
	Transition_to_Slide,
	Slide_Loop
};

UENUM(BlueprintType)
enum class E_MovementDirection : uint8
{
	Forward,
	Backward,
	Left,
	Right
};

/** 캐릭터 회전 방식 제어 */
UENUM(BlueprintType)
enum class E_RotationMode : uint8
{
	VelocityDirection,
	LookingDirection,
	Aiming
};

/** Chooser 컨텍스트 방향 */
UENUM(BlueprintType)
enum class E_ContextObjectDirection : uint8
{
	None,
	Forward,
	Backward,
	Left,
	Right
};

/** 애니메이션 사운드(Foley)를 위한 발 위치 구분 */
UENUM(BlueprintType)
enum class E_FoleyEventSide : uint8
{
	Left,
	Right
};
