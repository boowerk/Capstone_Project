---
memoc: true
type: state
scope: project-memory
created: 2026-05-21T07:03:24
updated: 2026-05-21T07:03:24
status: active
tags:
  - memoc
  - memoc/state
---
# Decisions

Durable project decisions live here. Keep entries short, dated, and useful to future agents.

## Decision Log

### 2026-05-20
- Use `UEFNSourceMesh` as the runtime animation-driving source and let `CharacterMesh0` (`MaskMan`) receive pose via `Retarget Pose From Mesh`, instead of driving MaskMan first.
- Reuse existing UEFN mannequin pose-search assets first; start with a minimal source AnimBP and a single existing PSD before building a fuller chooser/database stack.
- Prefer explicit runtime DB selection in `GP_CharacterAnimInstance` over the stock UEFN chooser for now, because chooser context wiring in this project/tooling path did not switch databases reliably in PIE.
- Prefer separate fixed `Motion Matching` nodes per locomotion state over hot-swapping the `Database` pin on a single node, because runtime DB replacement did not produce visible pose changes in PIE.
- Keep `CurrentMotionMatchState` enum as locomotion branch source of truth; remove temporary `bUse*MotionMatch` helper flags once enum-driven blending is wired.
- Prefer the Blueprint-added `CharacterTrajectoryComponent` on `BP_GP_PlayerCharacter` as trajectory source for `ABP_UEFNSource_Player` over the temporary C++ `GeneratedTrajectory` path when validating/debugging motion matching.
- To avoid fragile Blueprint nested-property wiring, bridge the Blueprint `CharacterTrajectoryComponent` into `GeneratedTrajectory` inside `UGP_CharacterAnimInstance` via reflection, then keep `GeneratedTrajectory -> Pose History` as the AnimGraph input path.
- Do not keep expanding bespoke locomotion branches as the long-term solution; chooser should remain the main database-selection authority once its expected context variables are restored.
- Restore the stock relaxed chooser by matching its input contract in `UGP_CharacterAnimInstance` (`MovementMode`, `Stance`, `MovementState`, `Gait`, direction flags, landing flags, and previous-frame state) rather than cloning chooser logic into code.
- For MaskMan locomotion, treat default movement speed (`500`) as run-family motion and sprint speed (`700`) as sprint-family motion; do not require a walk-family chooser branch in the first custom table pass.
- Build a new custom chooser rooted at `CHT_MM_MaskMan_Root` with embedded `Idle`, `Run`, `Sprint`, and `InAir` nested choosers, using the stock relaxed chooser layouts as templates instead of trying to salvage every original branch directly.
- Use a dedicated `SprintSpeedThreshold` (currently `650`) instead of reusing the broad run threshold, so chooser gait classification does not mark base speed `500` as sprint.
- **2026-05-23**: Elevate the `ContextData(0)` type of `CHT_MM_MaskMan_Root_OriginalStyle` (and other chooser tables) from the specific Blueprint class `ABP_UEFNSource_Player_C` to the common C++ parent class `GP_CharacterAnimInstance`. This prevents type mismatch errors (`expects ABP_UEFNSource_Player_C, but ABP_MaskMan_Player_C was passed in`) when evaluating motion matching for the MaskMan player.
- **2026-05-23**: 제자리 회전(Turn In Place) 상태와 대기(Idle) 상태가 교차하며 발생하는 애니메이션 버벅거림(Chattering) 현상을 제어하기 위해, 최소 턴 유지 시간(TurnInPlaceMinDuration = 0.6초) 및 모션 매칭 턴 애니메이션 에셋 이름 분석 기반 잠금(Animation Lock) 로직을 도입하여 부드러운 제자리 회전 연출을 유지하도록 결정함.
- **2026-05-23**: 마우스(카메라) 회전량에 따른 제자리 회전 시, 애니메이션 루트 모션 회전의 한계와 미세 각도 불일치로 인한 어긋남 문제를 근절하기 위해, (1) 회전 상태 탈출 각도 차이를 기존 5도에서 2도(`ActiveTurnThreshold = 2.0f`)로 타이트하게 조율하여 정렬 수준을 강화하고, (2) `GP_PlayerCharacter::Tick` 내부에서 제자리 상태일 때 애니메이션 루트 모션 회전을 반영하는 동시에 카메라 방향으로 미세 각도 편차(15도 이내)가 남았을 시 `RInterpTo`로 부드럽게 정밀 수렴(Fine Alignment)하는 수동 보정 제어를 추가하기로 결정함.
- **2026-05-23**: 모션 매칭 궤적 생성기(Trajectory Generator)가 제자리에서 마우스(카메라) 회전량을 올바르게 인지하여 최적의 턴인플레이스 에셋을 검색할 수 있도록, 누락되었던 `DesiredControllerYawLastUpdate` 변수를 틱당 `DesiredYaw` 값으로 실시간 갱신하는 로직을 추가함.
- **2026-05-23**: 제자리 회전 도중에 인위적인 회전 Interp를 혼합할 경우 디딤발과 물리각이 어긋나 발이 미끄러지는 현상(Slippage)을 차단하기 위해, 턴인플레이스가 구동 중일 때는 100% 순수 루트 모션 회전에 의존하고, 턴 모션이 종료된 직후 남은 미세 오차(10도 이내) 상황에서만 은은하게 `RInterpTo`로 카메라 정방향에 밀착 잠금(Lock)하도록 설계를 세분화함.
- **2026-05-23**: 기존 35번의 물리-애니메이션 슬립 예방 설계를 바탕으로 `GP_PlayerCharacter::Tick` 내부의 턴인플레이스 회전 로직을 최종 보완함. `bInTurnInPlace`가 참인 동안 애니메이션 루트 모션 회전(`RootMotionDeltaRot`)을 액터에 100% 온전히 누적 반영(`AddActorWorldRotation`)하는 동시에, 실시간 카메라 Yaw 방향으로의 미세 편차 추종을 위해 부드러운 감도(`InterpSpeed = 4.0f`)의 `RInterpTo` 보정 회전을 하이브리드로 결합함. 또한 턴 상태가 해제된 직후(`bInTurnInPlace == false`이고 속도가 0인 대기 상태일 때)에도 남은 미세 오차(15도 이내)에 대해 `RInterpTo(InterpSpeed = 3.0f)` 수렴 밀착을 수행하여, 마우스 회전 후 턴인플레이스에서 Idle로 블렌딩되는 시점의 어색함이나 애매한 어긋남을 완전히 근절하도록 최종 결정함.
- **2026-05-23**: 턴인플레이션이 트리거되는 즉시 C++ RInterpTo 회전이 개입하면 애니메이션의 예비 동작(Anticipation) 단계에서 발이 미끄러지는 현상(Initial Slide)이 유발됨을 확인하고, 턴 시작 후 `0.25초` 이내의 초반 구간 동안은 인위적인 수동 C++ 회전 보정을 전면 차단하고 오로지 애니메이션 자체의 순수 루트 모션 회전력에만 의존하도록 개선함. 디딤발 회전이 본격적으로 시작되는 `0.25초` 이후 중/반부 시점부터 보정 각도(`InterpSpeed = 5.0f`)를 부드럽게 가미하여, 초기 발 슬립 현상을 0% 완벽히 제거함과 동시에 최종 1:1 정렬 성능을 확보하는 구조로 튜닝하기로 결정함.
- **2026-05-23**: 위의 지연 보정 도입 이후에도 마우스를 미세하게 조금씩 회전시키는 상황에서 액터 로테이션이 강제로 카메라 방향을 야금야금 따라가며 아주 미세하게 발이 미끄러지는 현상(Micro-slippage)이 여전히 감지됨을 인지함. 이를 완벽히 근절하기 위해 AAA급 품질의 Yaw Hysteresis & One-shot Snap 기법을 도입하여, (1) 제자리 대기(Idle) 중에는 수동 RInterpTo 회전 보정을 100% 완전히 차단(OFF)하여 액터 회전을 칼같이 고정함으로써 발 미끄러짐을 0% 원천 봉쇄하고, (2) 오직 턴인플레이스가 정상 활성화되었을 때만 루트 모션으로 자연스럽게 돌게 하다가, 턴이 거의 마무리되는 후반부(`0.4초` 경과 시점)에만 고속 회전(`InterpSpeed = 12.0f`)을 주어 카메라 정면 방향으로 단 한 번에 빠르고 완벽하게 스냅(Snap to Camera) 정렬하도록 최종 보완함.

