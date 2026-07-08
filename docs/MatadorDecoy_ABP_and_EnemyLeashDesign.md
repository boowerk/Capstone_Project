# Matador Decoy ABP / Enemy Leash Design

## 목적

이 문서는 에디터에서 직접 `ABP_MatadorDecoy` 상태머신을 만들 때 참고하기 위한 작업 가이드다.

추가로, 일반 몬스터가 자기 구역을 벗어났을 때 즉시 생성 위치를 향해 휙 돌아가는 조잡한 복귀 연출을 개선하기 위한 AI/애니메이션 설계도 함께 정리한다.

---

## 1. ABP_MatadorDecoy 기본 방향

### 핵심 원칙

- 공격 몽타주는 이미 작동하므로 `DefaultSlot` 위에 얹는다.
- 로코모션과 방향 정리, 순간이동 후 자세 잡기는 상태머신이 담당한다.
- 임시 애니메이션은 Idle/Walk를 재사용해도 된다.
- 나중에 좋은 애니메이션이 생기면 상태 이름과 전환 조건은 유지하고 각 상태의 Sequence만 교체한다.

### 추천 AnimGraph 구조

```text
LocomotionStateMachine
  -> DefaultSlot
  -> Output Pose
```

`DefaultSlot`은 반드시 최종 `Output Pose` 직전에 둔다.

공격 몽타주가 기본 자세 위에 정상적으로 올라타려면 아래 구조가 유지되어야 한다.

```text
기본 포즈 / 로코모션
  -> DefaultSlot
  -> Output Pose
```

---

## 2. ABP_MatadorDecoy에서 사용할 변수

### C++ AnimInstance에서 이미 내려오는 변수

```text
GroundSpeed
bIsWalkingPressure
CurrentTarget
PressureState
bTeleportRequested
bPostTeleportAttackLocked
StepThrustIndex
PresentationState
PresentationDirection
PresentationStateTime
PresentationStateDuration
bRapierAimActive
bRapierLockedActive
bRapierThrustActive
bCapePrepareActive
bCapeLockedActive
bCapeGustActive
```

### ABP에서 추가하면 좋은 계산 변수

```text
bHasTarget
TargetYawDelta
AbsTargetYawDelta
TurnDirectionSign
bShouldTurnInPlace
bShouldFaceTargetAdjust
bShouldWalkStart
bShouldWalkStop
bRecentlyTeleported
```

### TargetYawDelta 의미

```text
-180 ~ 180

양수: 오른쪽으로 돌아야 함
음수: 왼쪽으로 돌아야 함
0에 가까움: 이미 타겟을 바라봄
```

권장 계산:

```text
TargetDirection = CurrentTarget.Location - Owner.Location
TargetYaw = TargetDirection.Rotation.Yaw
OwnerYaw = Owner.ActorRotation.Yaw
TargetYawDelta = NormalizeAxis(TargetYaw - OwnerYaw)
AbsTargetYawDelta = Abs(TargetYawDelta)
TurnDirectionSign = Sign(TargetYawDelta)
```

---

## 3. 1차 Locomotion State Machine

처음부터 너무 복잡하게 만들 필요는 없지만, 최소 Idle/Walk만으로 끝내면 디코이가 너무 장난감처럼 보인다.

권장 1차 상태는 아래와 같다.

```text
Entry
  -> Idle
  -> TurnInPlace
  -> WalkStart
  -> WalkPressure
  -> WalkStop
  -> FaceTargetAdjust
  -> TeleportIn
  -> PostTeleportReady
```

---

## 4. 상태별 설계

### Idle

평상시 대기 상태.

임시 애니메이션:

```text
MM_Matador_M_Relaxed_Stand_Idle_Loop
```

전환:

```text
Idle -> TurnInPlace
bHasTarget && AbsTargetYawDelta > 60 && GroundSpeed <= 5
```

```text
Idle -> WalkStart
bIsWalkingPressure && GroundSpeed > 5 && AbsTargetYawDelta <= 75
```

```text
Idle -> FaceTargetAdjust
bHasTarget && AbsTargetYawDelta > 20 && AbsTargetYawDelta <= 60
```

---

### TurnInPlace

제자리에서 타겟 방향으로 몸을 돌리는 상태.

임시 애니메이션:

```text
Idle 재사용
```

나중에 교체할 애니메이션:

```text
TurnLeft90
TurnRight90
TurnLeft180
TurnRight180
```

전환:

```text
TurnInPlace -> Idle
AbsTargetYawDelta <= 20 && !bIsWalkingPressure
```

```text
TurnInPlace -> WalkStart
bIsWalkingPressure && AbsTargetYawDelta <= 75
```

설계 메모:

- 실제 Actor 회전은 AI/Movement/C++ 쪽에서 처리해도 된다.
- ABP는 회전 동작을 보여주는 역할로 시작해도 충분하다.
- Turn 애니가 없으면 Idle을 임시로 쓰되, 이 상태 자체는 만들어두는 편이 좋다.

---

### FaceTargetAdjust

짧게 몸 방향을 다시 맞추는 상태.

`TurnInPlace`보다 작은 각도 보정에 사용한다.

임시 애니메이션:

```text
Idle 또는 Walk
```

나중에 교체할 애니메이션:

```text
AdjustStepLeft
AdjustStepRight
SmallPivotLeft
SmallPivotRight
```

전환:

```text
FaceTargetAdjust -> Idle
!bIsWalkingPressure && AbsTargetYawDelta <= 15
```

```text
FaceTargetAdjust -> WalkPressure
bIsWalkingPressure && AbsTargetYawDelta <= 35
```

```text
FaceTargetAdjust -> TurnInPlace
AbsTargetYawDelta > 75
```

설계 메모:

- 걷기 중 타겟이 옆으로 빠졌을 때 바로 미끄러지듯 꺾지 않기 위한 완충 상태다.
- 마타도르 디코이는 “천천히 압박하는 검객”이므로 방향 보정이 눈에 보여야 한다.

---

### WalkStart

걷기 시작 상태.

임시 애니메이션:

```text
MM_Matador_M_Neutral_Walk_Loop_F
```

나중에 교체할 애니메이션:

```text
WalkStartForward
WalkStartLeft
WalkStartRight
```

전환:

```text
WalkStart -> WalkPressure
TimeRemainingRatio < 0.15
```

또는 임시로:

```text
WalkStart -> WalkPressure
GroundSpeed > 30
```

설계 메모:

- 전용 start 애니가 없으면 바로 Walk를 써도 된다.
- 그래도 상태를 분리해두면 나중에 애니 교체가 쉽다.

---

### WalkPressure

타겟을 향해 천천히 압박하는 기본 이동 상태.

임시 애니메이션:

```text
MM_Matador_M_Neutral_Walk_Loop_F
```

전환:

```text
WalkPressure -> WalkStop
!bIsWalkingPressure || GroundSpeed <= 5
```

```text
WalkPressure -> FaceTargetAdjust
AbsTargetYawDelta > 45
```

```text
WalkPressure -> TurnInPlace
GroundSpeed <= 5 && AbsTargetYawDelta > 60
```

설계 메모:

- 가능하면 상태 내부에서 Orientation Warping 또는 Rotate Root Bone을 사용한다.
- 단, 워핑이 과하면 발이 미끄러질 수 있으므로 큰 각도는 `TurnInPlace`로 넘기는 편이 좋다.

---

### WalkStop

압박 이동을 멈추는 상태.

임시 애니메이션:

```text
Idle
```

나중에 교체할 애니메이션:

```text
WalkStopShort
WalkStopLong
```

전환:

```text
WalkStop -> Idle
TimeRemainingRatio < 0.15
```

임시 전환:

```text
WalkStop -> Idle
GroundSpeed <= 3
```

설계 메모:

- 정지 애니가 없으면 Idle로 바로 넘어가도 된다.
- 하지만 상태 자체는 분리해두는 것이 좋다.

---

### TeleportIn

순간이동 직후 등장 상태.

임시 애니메이션:

```text
Idle
```

나중에 교체할 애니메이션:

```text
TeleportAppear
TeleportRecover
```

전환:

```text
Any -> TeleportIn
PressureState == PostTeleportGrace
```

```text
TeleportIn -> PostTeleportReady
TimeRemainingRatio < 0.2
```

설계 메모:

- 순간이동 직후 바로 공격하면 불공정해 보인다.
- 등장 이펙트, 시선 정리, 검 발광 같은 텔레그래프가 이 구간에 들어가면 좋다.

---

### PostTeleportReady

순간이동 후 공격 금지 유예시간 동안 자세를 잡는 상태.

임시 애니메이션:

```text
Idle
```

나중에 교체할 애니메이션:

```text
ReadyPose
SwordGlowPose
```

전환:

```text
PostTeleportReady -> Idle
!bPostTeleportAttackLocked && !bIsWalkingPressure
```

```text
PostTeleportReady -> WalkPressure
!bPostTeleportAttackLocked && bIsWalkingPressure
```

---

## 5. 공격 몽타주와 상태머신의 관계

공격은 상태머신에 넣지 않고 `DefaultSlot`으로 처리한다.

```text
LocomotionStateMachine
  -> DefaultSlot
  -> Output Pose
```

다만 공격 직전 방향 보정은 상태머신이 도와줄 수 있다.

예시:

```text
Any -> FaceTargetAdjust
PresentationState == RapierAim && AbsTargetYawDelta > 20
```

주의:

- 공격 몽타주가 재생 중일 때 상태머신이 너무 강하게 회전 상태로 빠지면 상체/하체가 깨질 수 있다.
- 공격 몽타주 중에는 큰 상태 전환을 막고, 필요한 경우 `DefaultSlot` 몽타주가 우선권을 갖게 둔다.

---

## 6. 세걸음-찌르기 확장 상태

나중에 `StepThrust`를 제대로 만들면 아래 상태를 추가한다.

```text
RefinePose
StepForward
AttackReady
StrongBackStep
StrongTurn
StrongReady
```

권장 흐름:

```text
StepForward
  -> RefinePose
  -> StepForward
  -> RefinePose
  -> StepForward
  -> RefinePose
  -> AttackReady
  -> DefaultSlot 공격 몽타주
```

강화 찌르기:

```text
StrongBackStep
  -> RefinePose
  -> StrongBackStep
  -> RefinePose
  -> StrongBackStep
  -> RefinePose
  -> StrongTurn
  -> StrongReady
  -> DefaultSlot 강화 찌르기 몽타주
```

권장 C++/Ability 변수:

```text
StepThrustPhase

None
StepMove
Refine
Ready
Thrust
StrongBackStep
StrongTurn
StrongReady
StrongThrust
```

ABP 단독으로 세걸음 흐름 전체를 관리하기보다는, Ability/C++가 단계 변수를 내려주고 ABP는 그 단계에 맞는 자세를 보여주는 편이 안전하다.

---

## 6-1. 전환 조건을 C++ Bool로 관리하는 방식

ABP 상태머신의 Transition Rule에 복잡한 조건식을 직접 넣으면 관리가 어렵다.

권장 방식은 아래처럼 역할을 나누는 것이다.

```text
ABP:
상태 노드와 전환 선만 만든다.
Transition Rule에는 bool 변수 하나만 연결한다.

C++ / AnimInstance:
실제 전환 조건을 계산한다.
전환 bool을 매 프레임 갱신한다.
```

예시:

```text
Idle -> TurnInPlace
Transition Rule: bCan_Idle_To_TurnInPlace
```

```text
WalkPressure -> WalkStop
Transition Rule: bCan_WalkPressure_To_WalkStop
```

이렇게 하면 에디터에서는 연결만 하고, 조건 검토와 수정은 코드에서 할 수 있다.

### 디코이 1차 전환 bool

```text
bCan_Idle_To_TurnInPlace
bCan_Idle_To_FaceTargetAdjust
bCan_Idle_To_WalkStart

bCan_TurnInPlace_To_Idle
bCan_TurnInPlace_To_WalkStart

bCan_FaceTargetAdjust_To_Idle
bCan_FaceTargetAdjust_To_WalkPressure
bCan_FaceTargetAdjust_To_TurnInPlace

bCan_WalkStart_To_WalkPressure

bCan_WalkPressure_To_WalkStop
bCan_WalkPressure_To_FaceTargetAdjust
bCan_WalkPressure_To_TurnInPlace

bCan_WalkStop_To_Idle

bCan_Any_To_TeleportIn
bCan_TeleportIn_To_PostTeleportReady
bCan_PostTeleportReady_To_Idle
bCan_PostTeleportReady_To_WalkPressure
```

### 계산 예시

```text
bCan_Idle_To_TurnInPlace =
    bHasTarget
    && AbsTargetYawDelta > 60
    && GroundSpeed <= 5
    && !bIsAnySkillPreRollActive
```

```text
bCan_Idle_To_WalkStart =
    bIsWalkingPressure
    && AbsTargetYawDelta <= 75
    && !bPostTeleportAttackLocked
```

```text
bCan_WalkPressure_To_WalkStop =
    !bIsWalkingPressure
    || GroundSpeed <= 5
```

주의:

```text
C++이 ABP의 현재 상태를 직접 강제하지 않는다.
C++은 전환 가능 bool만 제공한다.
ABP 상태머신이 현재 상태와 연결된 선을 통해 실제 전환을 결정한다.
```

이 방식은 디코이뿐 아니라 이후 모든 몬스터에 확장할 수 있다.

---

## 6-2. 기술별 프리롤/완충 상태 설계

몽타주가 작동하더라도 바로 재생하면 기본 자세에서 공격 자세로 튀어 보일 수 있다.

따라서 기술별로 아래 구조를 둔다.

```text
기본 상태
  -> 기술 예고 상태
  -> 기술 대기/완충 상태
  -> 기술 몽타주
  -> 기술 회복 상태
  -> 기본 상태
```

예시:

```text
Idle / WalkPressure
  -> RapierPreRoll
  -> RapierReadyHold
  -> DefaultSlot: RapierThrust Montage
  -> RapierRecover
  -> Idle / WalkPressure
```

### 왜 필요한가

```text
기본 Idle에서 바로 찌르기 몽타주가 나오면 포즈 점프가 보임
걷는 도중 바로 공격하면 하체/상체가 부자연스러움
타겟 방향이 틀어진 상태에서 공격하면 허공을 찌르는 느낌이 남
```

프리롤/완충 상태를 넣으면 아래 흐름이 된다.

```text
몸 방향 정리
무기 자세 준비
짧은 대기
공격 몽타주
회복
```

플레이어는 공격을 예측할 수 있고, 애니메이션도 덜 튄다.

---

## 6-3. 디코이 기술 상태 예시

### RapierThrust

권장 상태:

```text
RapierPreRoll
RapierReadyHold
RapierMontageActive
RapierRecover
```

흐름:

```text
Any Locomotion State
  -> FaceTargetAdjust
  -> RapierPreRoll
  -> RapierReadyHold
  -> DefaultSlot: AM_MatadorDecoy_RapierThrust
  -> RapierRecover
  -> Idle 또는 WalkPressure
```

임시 애니메이션:

```text
RapierPreRoll = Idle
RapierReadyHold = Idle
RapierRecover = Idle
```

나중에 교체할 애니메이션:

```text
RapierPreRoll = 검을 세우며 준비
RapierReadyHold = 찌르기 직전 정지 자세
RapierRecover = 찌른 뒤 균형 회복
```

전환 bool:

```text
bCan_Any_To_RapierPreRoll
bCan_RapierPreRoll_To_RapierReadyHold
bCan_RapierReadyHold_To_RapierMontage
bCan_RapierMontage_To_RapierRecover
bCan_RapierRecover_To_Idle
bCan_RapierRecover_To_WalkPressure
```

계산 기준:

```text
bCan_Any_To_RapierPreRoll =
    PresentationState == RapierAim
    && AbsTargetYawDelta <= 35
```

```text
bCan_RapierReadyHold_To_RapierMontage =
    PresentationState == RapierThrust
```

```text
bCan_RapierMontage_To_RapierRecover =
    !bRapierThrustActive
    || !Montage_IsPlaying(RapierThrustMontage)
```

---

### CapeGust

권장 상태:

```text
CapePreRoll
CapeReadyHold
CapeMontageActive
CapeRecover
```

흐름:

```text
Any Locomotion State
  -> FaceTargetAdjust
  -> CapePreRoll
  -> CapeReadyHold
  -> DefaultSlot: AM_MatadorDecoy_CapeGust
  -> CapeRecover
  -> Idle 또는 WalkPressure
```

임시 애니메이션:

```text
CapePreRoll = Idle
CapeReadyHold = Idle
CapeRecover = Idle
```

나중에 교체할 애니메이션:

```text
CapePreRoll = 망토를 들어 올림
CapeReadyHold = 망토를 펼친 대기 자세
CapeRecover = 망토를 내리며 복귀
```

전환 bool:

```text
bCan_Any_To_CapePreRoll
bCan_CapePreRoll_To_CapeReadyHold
bCan_CapeReadyHold_To_CapeMontage
bCan_CapeMontage_To_CapeRecover
bCan_CapeRecover_To_Idle
bCan_CapeRecover_To_WalkPressure
```

계산 기준:

```text
bCan_Any_To_CapePreRoll =
    PresentationState == CapePrepare
    && AbsTargetYawDelta <= 45
```

```text
bCan_CapeReadyHold_To_CapeMontage =
    PresentationState == CapeGust
```

---

## 6-4. 기술 프리롤과 몽타주 호출 타이밍

기술 몽타주는 C++에서 이벤트가 오자마자 무조건 재생하면 포즈 완충이 어렵다.

더 좋은 구조:

```text
Ability/C++:
기술 요청 상태만 세팅

ABP/C++ AnimInstance:
프리롤 상태 진입
ReadyHold 상태까지 도달
몽타주 재생 허용 bool이 true가 되면 몽타주 재생
```

권장 변수:

```text
RequestedSkillPresentation
ActiveSkillPresentation
bSkillPreRollActive
bSkillReadyHoldActive
bSkillMontageRequested
bSkillMontageStarted
bSkillMontageFinished
bSkillRecoverActive
```

처음에는 디코이에만 적용한다.

```text
RequestedSkillPresentation = Rapier
  -> RapierPreRoll
  -> RapierReadyHold
  -> Play Rapier Montage
  -> RapierRecover
```

나중에 모든 몬스터에 일반화할 때는 몬스터마다 아래 데이터만 다르게 둔다.

```text
SkillId
PreRollState
ReadyHoldState
Montage
RecoverState
MinPreRollTime
ReadyHoldTime
RecoverTime
FacingYawLimit
```

---

## 6-5. 몬스터 공통 확장 방향

디코이에서 검증되면 모든 몬스터에 아래 공통 패턴을 적용할 수 있다.

```text
Monster AnimInstance
  - 전환 bool 계산
  - 방향/타겟 각도 계산
  - 스킬 프리롤/완충/회복 상태 계산

Monster ABP
  - 상태머신 노드와 선
  - 각 선에는 bool 하나만 연결
  - 각 상태에는 임시 또는 전용 애니메이션 배치

Ability / AI
  - 어떤 기술을 쓸지 결정
  - 기술 요청을 AnimInstance로 전달
```

공통 상태 이름:

```text
Idle
TurnInPlace
FaceTargetAdjust
WalkStart
WalkLoop
WalkStop
SkillPreRoll
SkillReadyHold
SkillMontageActive
SkillRecover
HitReact
Death
LeashGiveUp
ReturnHome
HomeRecenter
```

장점:

```text
ABP 조건식 실수 감소
코드 리뷰 가능
디버그 로그 가능
몬스터마다 상태 이름과 규칙 통일 가능
애니메이션 교체가 쉬움
```

단점:

```text
초기 bool 변수가 많아짐
상태 이름 규칙을 지켜야 함
C++과 ABP 상태선 이름이 어긋나면 디버깅 필요
```

따라서 처음에는 디코이에만 적용하고, 검증 후 일반 몬스터로 확장한다.

---

## 7. 일반 몬스터 구역 복귀 문제

### 현재 문제

일반 몬스터가 자기 구역을 벗어나면 즉시 원래 자리로 돌아가려 한다.

문제는 복귀 결정이 너무 즉각적이고, 방향 전환이나 인지 동작 없이 생성 위치를 향해 바로 몸을 돌린다는 점이다.

플레이어 체감:

```text
몬스터가 추격 중이다.
구역 경계를 넘었다.
몬스터가 갑자기 나를 무시한다.
몸을 휙 돌린다.
생성 위치로 기계처럼 돌아간다.
```

소울라이크류 기준으로는 너무 시스템이 드러난다.

---

## 8. 권장 복귀 연출 철학

구역 복귀는 단순 이동 명령이 아니라 하나의 상태 연출이어야 한다.

플레이어가 느껴야 하는 흐름:

```text
몬스터가 너무 멀리 끌려나왔다.
잠깐 멈춘다.
플레이어를 경계하거나 으르렁거린다.
추격을 포기하는 듯한 몸짓을 한다.
천천히 몸을 돌린다.
자기 구역으로 돌아간다.
돌아간 뒤 다시 경계 자세를 잡는다.
체력/전투 상태가 정리된다.
```

핵심은 “AI가 끊긴 것”처럼 보이지 않게 만드는 것이다.

---

## 9. Enemy Leash Return 상태 설계

현재 프로젝트에는 이미 아래 개념이 있다.

```text
HomeLocation
bShouldReturnHome
DistanceFromHome
bLeashReturnHomeActive
```

이 위에 복귀 연출 상태를 얹는 것을 추천한다.

### 권장 상태

```text
Combat
  -> LeashWarning
  -> LeashGiveUp
  -> TurnTowardHome
  -> ReturnHome
  -> HomeRecenter
  -> ResumeGuard
```

---

### LeashWarning

구역을 살짝 벗어난 상태.

아직 바로 복귀하지 않는다.

동작:

```text
추격은 계속하되 공격 빈도 감소
플레이어가 더 멀어지면 복귀 준비
짧은 경고 모션 또는 멈칫
```

조건:

```text
DistanceFromHome > LeashSoftRadius
```

추천 수치:

```text
LeashSoftRadius = 기존 복귀 반경의 80~90%
LeashWarningDuration = 0.4~1.0초
```

---

### LeashGiveUp

추격 포기를 플레이어에게 보여주는 상태.

동작:

```text
이동 정지
플레이어를 0.3~0.6초 바라봄
공격 캔슬 또는 공격 금지
짧은 울음/자세 낮춤/무기 내림
```

조건:

```text
DistanceFromHome > LeashHardRadius
또는
LeashWarning 상태가 일정 시간 지속됨
```

추천 수치:

```text
GiveUpDuration = 0.4~0.8초
AttackLockDuringGiveUp = true
```

---

### TurnTowardHome

바로 홈 방향으로 휙 돌지 않고, 회전 상태를 명시적으로 둔다.

동작:

```text
MoveTo 시작 전 회전만 수행
HomeLocation 방향으로 몸을 돌림
회전 속도 제한
큰 각도면 Turn 애니 사용
```

조건:

```text
AbsHomeYawDelta > 20
```

추천 수치:

```text
ReturnTurnRate = 180~360 deg/sec
ReturnTurnAcceptAngle = 10~20도
MaxTurnTowardHomeTime = 1.0초
```

중요:

```text
이 상태에서는 MoveTo(HomeLocation)를 아직 시작하지 않는다.
```

---

### ReturnHome

실제 홈 위치로 복귀하는 상태.

동작:

```text
MoveTo(HomeLocation)
플레이어 타겟 잠금 해제
공격 금지
피격 반응은 허용하되 재추격은 제한
```

이동 속도:

```text
일반 몬스터: 걷기 또는 느린 달리기
빠른 몬스터: 전용 복귀 조깅
보스/강적: 위엄 있는 걷기
```

추천 수치:

```text
ReturnHomeAcceptanceRadius = 80~150
ReturnHomeSpeedScale = 0.6~0.9
```

주의:

- 복귀 중 플레이어가 다시 가까워졌다고 즉시 추격으로 튀면 안 된다.
- 최소 복귀 시간 또는 홈 근처 도착 조건을 둔다.

---

### HomeRecenter

홈 근처에 도착한 뒤 원래 방향/순찰 방향을 다시 잡는 상태.

동작:

```text
MoveTo 종료
정지 모션
SpawnRotation 또는 PatrolForward 방향으로 회전
전투 타겟 초기화
필요하면 체력/상태 회복
```

추천 수치:

```text
HomeRecenterDuration = 0.3~0.8초
HomeFacingAcceptAngle = 15도
```

---

### ResumeGuard

복귀 완료 후 경계 상태.

동작:

```text
Idle 또는 Patrol 복귀
Perception 재활성화
새 타겟 평가 허용
```

조건:

```text
HomeRecenter 완료
```

---

## 10. Leash 반경을 두 단계로 나누기

즉시 복귀가 조잡해 보이는 가장 큰 이유는 경계가 하나뿐이기 때문이다.

추천:

```text
LeashSoftRadius
LeashHardRadius
LeashReengageRadius
```

### LeashSoftRadius

경고 구간.

```text
DistanceFromHome > Soft
```

이때는 아직 복귀하지 않고, 공격/추격을 약하게 제한한다.

### LeashHardRadius

진짜 복귀 시작.

```text
DistanceFromHome > Hard
```

이때 `LeashGiveUp`으로 들어간다.

### LeashReengageRadius

복귀 후 다시 전투 가능한 안쪽 반경.

```text
DistanceFromHome < Reengage
```

추천:

```text
ReengageRadius = SoftRadius * 0.7~0.8
```

이렇게 히스테리시스를 둬야 경계선에서 추격/복귀가 떨리지 않는다.

---

## 11. 애니메이션 구성

### 임시 구성

전용 애니가 없어도 아래처럼 상태를 먼저 만들 수 있다.

```text
LeashWarning = Idle
LeashGiveUp = Idle
TurnTowardHome = Idle 또는 TurnInPlace
ReturnHome = Walk 또는 Jog
HomeRecenter = Idle
ResumeGuard = Idle
```

### 나중에 교체할 애니메이션

```text
AlertLookBack
GiveUpGrowl
WeaponLower
TurnBackLeft90
TurnBackRight90
TurnBack180
ReturnWalk
ReturnJog
HomeStop
RecenterIdle
```

---

## 12. BT / AIController / AnimBP 책임 분리

### AIController 또는 Service

담당:

```text
DistanceFromHome 계산
Soft/Hard/Reengage 판정
bShouldReturnHome 세팅
Leash 상태 enum 관리
타겟 유지/해제 정책 결정
```

### Behavior Tree

담당:

```text
Leash 상태별 Task 실행
MoveTo(HomeLocation)
Wait(GiveUpDuration)
RotateToFace(HomeLocation)
HomeRecenter Wait
```

### AnimBP

담당:

```text
Leash 상태에 맞는 자세/회전/복귀 애니 재생
Turn 방향에 따른 좌/우 애니 선택
ReturnHome 이동 중 Walk/Jog 재생
```

### CharacterMovement

담당:

```text
복귀 중 이동 속도 제한
복귀 중 OrientRotationToMovement 설정
회전 속도 제한
```

---

## 13. 권장 Blackboard 키

기존 키:

```text
HomeLocation
bShouldReturnHome
DistanceFromHome
```

추가 추천:

```text
LeashState
LeashSoftExceeded
LeashHardExceeded
ReturnHomeStartedTime
HomeYawDelta
bLeashAttackLocked
bCanReengageAfterReturn
```

`LeashState` enum 예시:

```text
None
Warning
GiveUp
TurnTowardHome
Returning
Recenter
Cooldown
```

---

## 14. 재교전 정책

복귀 중 플레이어가 따라와서 때릴 수 있다.

이때 선택지는 세 가지다.

### Option A. 홈 도착 전까지 무조건 복귀

장점:

```text
시스템이 단순함
경계선 악용 방지
```

단점:

```text
플레이어가 때려도 무시하는 느낌이 날 수 있음
```

### Option B. 큰 피해를 받으면 잠깐 반응만 하고 복귀 유지

추천.

장점:

```text
맞았을 때 반응은 있어서 덜 어색함
그래도 leash 목적은 유지됨
```

단점:

```text
HitReact와 ReturnHome 상태 충돌 처리 필요
```

### Option C. 홈 근처로 돌아온 뒤 재교전 허용

장점:

```text
자연스러움
플레이어가 따라오면 다시 싸움 가능
```

단점:

```text
복귀 완료 판정과 재교전 반경 설계 필요
```

추천 조합:

```text
B + C
```

복귀 중 피격 반응은 허용하되 추격 재개는 막고, 홈 근처로 돌아온 뒤 다시 플레이어가 시야/거리 안에 있으면 재교전한다.

---

## 15. 최종 추천 구현 순서

### 1단계: 연출 없는 로직 개선

```text
SoftRadius / HardRadius / ReengageRadius 분리
HardRadius 진입 시 바로 MoveTo하지 않고 GiveUpDelay 추가
ReturnHome 중 공격 금지
Home 근처 도착 후 재교전 허용
```

### 2단계: 회전 상태 추가

```text
GiveUp 후 TurnTowardHome 상태 추가
Home 방향을 바라본 뒤 MoveTo 시작
회전 속도 제한
```

### 3단계: AnimBP 상태 추가

```text
LeashWarning
LeashGiveUp
TurnTowardHome
ReturnHome
HomeRecenter
```

처음에는 Idle/Walk 재사용.

### 4단계: 전용 애니메이션 교체

```text
GiveUp
TurnBack
ReturnWalk
HomeStop
```

---

## 16. 성공 기준

### 마타도르 디코이 ABP

```text
디코이가 생성되면 Idle 재생
타겟 압박 중 Walk 재생
큰 각도 차이가 있으면 TurnInPlace 또는 FaceTargetAdjust를 거침
순간이동 후 바로 공격하지 않고 TeleportIn/PostTeleportReady를 거침
Rapier/Cape 공격은 DefaultSlot 위에서 재생
```

### 일반 몬스터 구역 복귀

```text
구역을 살짝 벗어나도 즉시 복귀하지 않음
HardRadius를 넘으면 추격 포기 동작을 보여줌
원위치를 향해 바로 순간 회전하지 않고 TurnTowardHome을 거침
복귀 중 공격하지 않음
홈 도착 후 잠깐 재정렬하고 경계 상태로 복귀
플레이어가 따라오면 홈 근처에서만 재교전
```
