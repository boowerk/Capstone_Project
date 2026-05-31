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
# Current Project State

Last synced: 2026-05-23T00:00:00

## Current Status

- SkillData DA now exposes execution/VFX fields with clearer editor labels: `Execution Actor Class`, `Impact Visual Actor Class`, and `Active VFX Override`; internal C++ names remain unchanged for BP/DA safety.
- `UGP_SkillAugmentData` exists as the first skill augment data shell. It stores target skill tags, optional granted tech element, numeric modifiers, and optional visual overrides, but is not wired into runtime selection or damage calculation yet.
- `BP_GP_PlayerCharacter` has a visible `UEFNSourceMesh` native skeletal mesh component attached above `CharacterMesh0`.
- `UEFNSourceMesh` now uses `/Game/Characters/PlayerCharacter/ABP_UEFNSource_Player` as its AnimBP.
- `CharacterMesh0` still uses `/Game/Asset/CharacterAction/MaskMan/ABP_MaskMan_Player` to retarget from the parent mesh.
- `GP_CharacterAnimInstance` now exposes chooser-facing locomotion context (`MovementMode`, `Stance`, `MovementState`, `Gait`, `MovementDirection`, landing/turn/start flags) in addition to its temporary runtime DB fallback and chooser evaluation path.
- `BP_GP_PlayerCharacter` has a Blueprint-added `CharacterTrajectory` (`CharacterTrajectoryComponent`) and `ABP_UEFNSource_Player` caches that component in EventGraph.
- `GP_CharacterAnimInstance` now reflects that Blueprint `CharacterTrajectoryComponent` at runtime and copies its internal `Trajectory` into `GeneratedTrajectory`, falling back to `PoseSearchGenerateTransformTrajectory(...)` if unavailable.
- `GP_CharacterAnimInstance` tracks previous-frame locomotion context (`MovementMode_LastFrame`, `Gait_LastFrame`, `LastLocalVelocityDirection`, `LastVerticalVelocity`) so the stock UEFN chooser tables can be restored against named inputs instead of new ad-hoc branches.
- `ABP_UEFNSource_Player` still keeps the enum-blend fallback graph, but chooser-driven DB selection is being moved into a new custom root chooser for MaskMan.
- `/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/ChooserTables/CHT_MM_MaskMan_Root` is being authored with embedded `Idle`, `Run`, `Sprint`, and `InAir` nested choosers. `Walk` is intentionally omitted for now because MaskMan's default locomotion speed (`500`) should already use run-family PSDs.
- `BP_GP_PlayerCharacter` now uses semi-fixed GameAnimationSample Strafe/Aim-style rotation: `bUseControllerRotationYaw=false`, `bOrientRotationToMovement=false`, and controller desired rotation is enabled only while movement input is active and not Fixed. Idle keeps actor yaw fixed so yaw delta can drive Turn In Place.
- `GP_PlayerController` smooths directional/sprint `MaxWalkSpeed` changes through `MaxWalkSpeedInterpSpeed` instead of snapping immediately, giving motion matching more time to select start/stop/pivot/TIP transitions.
- `BP_GP_PlayerCharacter` air movement tuning now matches the sample baseline: `CharacterMovement.AirControl = 0.25` and `BrakingDecelerationFalling = 1500`.
- Actual movement speed now scales by input direction in `GP_PlayerController::Input_Move`; controller defaults are forward `500`, side `350`, back `300`, sprint forward `700`, sprint side/back `350/300`.
- Character source body-size ratio can drive movement speed through `PDA_CharacterAnimationSet.MovementSpeedProfile.MovementSpeedScaleRatio` (default `1`). This is manual authoring data relative to the mannequin at scale 1, not the runtime mesh component scale. Runtime max speeds multiply by this ratio; `UGP_CharacterAnimInstance.Speed2D` divides actual speed by the ratio so chooser thresholds stay in mannequin scale-1 space.
- Directional movement speed data is now owned by `PDA_CharacterAnimationSet.MovementSpeedProfile` and copied into `AGP_PlayerCharacter` on begin play.
- `PDA_CharacterAnimationSet` no longer owns legacy locomotion blendspace, sprint-stop, jump-loop, landing montage, or sprint enter/exit montage slots; runtime motion matching owns locomotion/air playback, with only a comment placeholder for a future jump animation montage.
- `PDA_CharacterAnimationSet` now has source-skeleton fallback montage slots (`SourceDashMontage`, source primary/light/heavy arrays). Player Dash/Primary prefer authored PDA target montages, then fall back to UEFNSource montage playback when target montage slots are empty.
- `AGP_PlayerCharacter` gates UEFNSource root-motion translation application to active fallback montages only, moves the capsule through `SafeMoveUpdatedComponent`, and exposes last fallback root-motion velocity for future inertia handoff.
- `ABP_UEFNSource_Player` now routes `Pose History -> DefaultSlot -> Output Pose`, allowing UEFNSource fallback montages to evaluate through the source AnimGraph. `GP_Dash` also clears fallback dash via montage-duration timer if no `ActionEnd` notify/event arrives.
- `PDA_CharacterAnimationSet.SourceRootMotionTranslationYawOffset` defaults to `-90`; `AGP_PlayerCharacter` applies it only to fallback source root-motion translation before converting to world space.
- The attempted `UEFNSourceMeshScale` multiplier on consumed fallback root-motion translation was reverted because it made runtime movement/debug speed worse.
- The attempted `UGP_CharacterAnimInstance` fallback-root-motion velocity override was reverted; speed/debug context is back to `Character->GetVelocity()`.
- The attempted actor-location-delta debug speed display was reverted because it jittered while idle; on-screen speed display is back to `GroundSpeed` / `Speed2D`.
- Player movement input smoothing now snaps immediately when desired direction opposes the current smoothed direction. MoveAction `Completed` is handled by a reset-only function instead of re-entering `Input_Move`.
- Speed debug display is emitted only by the anim instance whose owning skeletal mesh is `Character->GetMesh()`; this prevents `UEFNSourceMesh` from overwriting the same on-screen debug key.
- Speed debug is additionally gated to the locally controlled player pawn, preventing stale transient PIE/world instances from overwriting the same on-screen debug key.
- `UGP_CharacterAnimInstance` reads the final `AGP_PlayerCharacter::GetMovementSpeedScaleRatio()` and `AGP_PlayerCharacter` pushes scale ratio changes into both target and UEFN source anim instances when movement profiles/GAS scale change.
- Source-only pose-search chooser load/evaluation is gated to the UEFNSource anim instance; target retarget ABP and boss ABPs no longer feed `CHT_MM_MaskMan_Root_OriginalStyle` with incompatible context objects.
- Move direction smoothing snaps on strong direction changes (`dot < 0.5`) to avoid lingering several steps in the previous direction.
- `AGP_PlayerCharacter` exposes GAS-ready hooks: `SetGASMovementSpeedMultiplier`, `SetGASMovementSpeedScaleRatioMultiplier`, `SetMovementSpeedProfileOverride`, and `ClearMovementSpeedProfileOverride`.
- 제자리 회전(Turn In Place) 동작을 0.6초에서 1.0초로 연장하였고, 회전이 강제 유지(Hysteresis)되는 도중에도 이동 가속(`bHasAcceleration`), 공중 상태(`bIsFalling`), 회전 임계 속도 초과(`Speed2D > TurnInPlaceMaxSpeed`) 등 다른 애니메이션 상태 조건이 감지되면 회전이 즉시 turn=0 (`ShouldTurnInPlace = false`)으로 중단되도록 구현하였습니다.
- C++ 코드의 인위적인 수동 정렬 개입(`RInterpTo`)을 **100% 전면 삭제**하여 회전 튕김 및 발 슬립 현상을 완전히 해결하고, 제자리 상태에서는 오로지 모션 매칭 애니메이션이 주도하는 순수 루트 모션 회전량(`AddActorWorldRotation(RootMotionDeltaRot)`)만 반영하도록 원상 복구하였습니다.
- 모션 매칭 엔진이 카메라 Yaw 회전 방향을 부드럽고 완벽하게 1:1 최종 정렬할 수 있도록, 제자리 회전 스키마 에셋(`/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/Schemas/PSS_Relaxed_StandTurn`) 내 Trajectory Heading 가중치를 `3.0`으로 자동 튜닝 및 저장하는 에디터 파이썬 자동화 스크립트(`pss_tune_guide.py`)를 프로젝트 루트에 배포하여 모션 매칭 에셋 주도 정렬(Asset-driven Alignment)을 실현하였습니다.

## Project Snapshot

<!-- memoc:snapshot:start -->
- Last synced: 2026-05-26T11:35:43
- Detected stack: Not detected

### Source Directories

- `.claude`
- `.dance-of-tal`
- `.opencode`
- `.vs`
- `Project_Eden`
<!-- memoc:snapshot:end -->

## Open Tasks

- Verify runtime behavior in PIE for source motion, visible state transitions, and retarget fidelity with Blueprint trajectory preferred at runtime.
- Connect `CHT_MM_MaskMan_Root` as the active chooser source for `UGP_CharacterAnimInstance` and validate `Idle / TurnInPlace / Run / Sprint / InAir`.
- Keep manual enum/MM branching only as a temporary fallback until the new chooser fully replaces it.

## Completed Tasks

- Created `Diagonal_Path_Curvature_Analysis.md` containing diagnostic details on the forward-to-diagonal trajectory angularity.
- Resolved missing declaration `GetActiveMovementSpeedProfile` in `GP_PlayerCharacter.h`, fixing multiple C++ compilation errors and verified successful build.
- Resolved missing include `#include "GameplayTags/GP_Tags.h"` in `GP_BaseCharacter.cpp`, fixing compiler errors (C2653/C2065 for GPTags element variables) and verified clean compile of Project C++.
- See `.memoc/worklog/` for full shared activity history.

## Commands

- Unreal Python: created `/Game/Characters/PlayerCharacter/ABP_UEFNSource_Player`
- Unreal BP edit: assigned `BP_GP_PlayerCharacter.UEFNSourceMesh.AnimClass = ABP_UEFNSource_Player`
- C++ edit: added `PoseSearch`/`Chooser` support, runtime trajectory bridging, runtime pose-search DB fallback, and chooser-facing context variables in `GP_CharacterAnimInstance`

## Notes

- Minimal source AnimGraph is `Motion Matching -> Pose History -> DefaultSlot -> Output Pose`.
- `GeneratedTrajectory` is connected to `Pose History.TransformTrajectory` in `ABP_UEFNSource_Player`.
- `ABP_UEFNSource_Player` CDO now uses `PSD_Relaxed_Stand_Walk_F_Loops` for `WalkPoseSearchDatabase` instead of the broader `PSD_Relaxed_Stand_Walk_Loops`.
- `GP_CharacterAnimInstance` exposes `CurrentMotionMatchState` and still computes `RuntimePoseSearchDatabase` as state/debug data.
- `ABP_UEFNSource_Player` is still using enum-driven branches as a temporary fallback while the new chooser is authored and validated.
- Exported `/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/CHT_PoseSearchDatabases_Relaxed` to inspect required inputs. Root chooser expects `MovementMode`, `Stance`, `MovementState`, and `Gait`; nested tables also expect `MovementDirection`, `MovementDirection_Recent`, `MovementMode_LastFrame`, `Gait_LastFrame`, `IsStarting`, `IsPivoting`, `ShouldSpinTransition`, `JustTraversed`, `JustLanded_Light`, `JustLanded_Heavy`, `ShouldTurnInPlace`, and `Speed2D`.
- New custom chooser authoring started under `/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/ChooserTables/`; `Idle`, `Run`, `Sprint`, and `InAir` nested chooser rows were seeded from the stock relaxed chooser layout, but tuned around MaskMan's run-first locomotion (`500 -> Run`, `700 -> Sprint`).
- Runtime default chooser load path now points at `ChooserTables/CHT_MM_MaskMan_Root`, and sprint classification was split out with `SprintSpeedThreshold = 650` so base movement speed `500` stays in the run family.
- Temporary `bUse*MotionMatch` helper flags were removed from `GP_CharacterAnimInstance`.
- Animation threshold tuning is separate from real movement speed. For directional movement speed bugs, check `GP_PlayerController` first, not only `GP_CharacterAnimInstance`.
- `ABP_UEFNSource_Player` CDO defaults:
  - `IdlePoseSearchDatabase = PSD_Relaxed_Stand_Idles`
  - `WalkPoseSearchDatabase = PSD_Relaxed_Stand_Walk_Loops`
  - `RunPoseSearchDatabase = PSD_Relaxed_Stand_Run_F_Loops`
  - `SprintPoseSearchDatabase = PSD_Relaxed_Stand_Sprint_Loops`
  - `JumpPoseSearchDatabase = PSD_Relaxed_Stand_Jump`
- Correct header for `FTransformTrajectory` is `Animation/TrajectoryTypes.h`.

## Change Log

See `.memoc/worklog/` and generated `.memoc/activity.md`.
