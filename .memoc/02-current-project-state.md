# Current Project State

Last synced: 2026-05-20T13:22:00

## Current Status

- `BP_GP_PlayerCharacter` has a visible `UEFNSourceMesh` native skeletal mesh component attached above `CharacterMesh0`.
- `UEFNSourceMesh` now uses `/Game/Characters/PlayerCharacter/ABP_UEFNSource_Player` as its AnimBP.
- `CharacterMesh0` still uses `/Game/Asset/CharacterAction/MaskMan/ABP_MaskMan_Player` to retarget from the parent mesh.
- `GP_CharacterAnimInstance` now generates trajectory data and selects `RuntimePoseSearchDatabase` each tick from speed/falling state.
- `BP_GP_PlayerCharacter` has a Blueprint-added `CharacterTrajectory` (`CharacterTrajectoryComponent`) and `ABP_UEFNSource_Player` caches that component in EventGraph.
- `ABP_UEFNSource_Player` no longer relies on runtime database hot-swapping on a single `Motion Matching` node.
- The source AnimGraph now contains five fixed source branches (idle / walk / run / sprint / jump) being reorganized around `CurrentMotionMatchState` enum selection before `Pose History`.

## Project Snapshot

<!-- context-forge:snapshot:start -->
- Last synced: 2026-05-19T13:03:01
- Detected stack: Not detected

### Source Directories

- `.claude`
- `.dance-of-tal`
- `.opencode`
- `.vs`
- `Project_Eden`
<!-- context-forge:snapshot:end -->

## Open Tasks

- Wire `CharacterTrajectory.trajectory` into `Pose History.TransformTrajectory` and verify runtime behavior in PIE for source motion, visible state transitions, and retarget fidelity.
- Tune `IdleSpeedThreshold`, `WalkSpeedThreshold`, and `RunSpeedThreshold` if switching feels wrong.
- Decide later whether to return to chooser-based selection or keep the explicit runtime DB switch path.

## Completed Tasks

See `.memoc/log.md` for full history.

## Commands

- Unreal Python: created `/Game/Characters/PlayerCharacter/ABP_UEFNSource_Player`
- Unreal BP edit: assigned `BP_GP_PlayerCharacter.UEFNSourceMesh.AnimClass = ABP_UEFNSource_Player`
- C++ edit: added `PoseSearch` module dependency, trajectory generation, and runtime pose-search DB selection in `GP_CharacterAnimInstance`

## Notes

- Minimal source AnimGraph is `Motion Matching -> Pose History -> Output Pose`.
- `GeneratedTrajectory` node currently exists in `ABP_UEFNSource_Player`, but `Pose History.TransformTrajectory` is currently unconnected.
- `GP_CharacterAnimInstance` exposes `CurrentMotionMatchState` and still computes `RuntimePoseSearchDatabase` as state/debug data.
- `ABP_UEFNSource_Player` is moving to enum-driven branch selection using `CurrentMotionMatchState`.
- Temporary `bUse*MotionMatch` helper flags were removed from `GP_CharacterAnimInstance`.
- `ABP_UEFNSource_Player` CDO defaults:
  - `IdlePoseSearchDatabase = PSD_Relaxed_Stand_Idles`
  - `WalkPoseSearchDatabase = PSD_Relaxed_Stand_Walk_Loops`
  - `RunPoseSearchDatabase = PSD_Relaxed_Stand_Run_F_Loops`
  - `SprintPoseSearchDatabase = PSD_Relaxed_Stand_Sprint_Loops`
  - `JumpPoseSearchDatabase = PSD_Relaxed_Stand_Jump`
- Correct header for `FTransformTrajectory` is `Animation/TrajectoryTypes.h`.

## Change Log

See `.memoc/log.md`.
