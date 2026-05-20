# Current Project State

Last synced: 2026-05-20T18:25:00

## Current Status

- `BP_GP_PlayerCharacter` has a visible `UEFNSourceMesh` native skeletal mesh component attached above `CharacterMesh0`.
- `UEFNSourceMesh` now uses `/Game/Characters/PlayerCharacter/ABP_UEFNSource_Player` as its AnimBP.
- `CharacterMesh0` still uses `/Game/Asset/CharacterAction/MaskMan/ABP_MaskMan_Player` to retarget from the parent mesh.
- `GP_CharacterAnimInstance` now exposes chooser-facing locomotion context (`MovementMode`, `Stance`, `MovementState`, `Gait`, `MovementDirection`, landing/turn/start flags) in addition to its temporary runtime DB fallback and chooser evaluation path.
- `BP_GP_PlayerCharacter` has a Blueprint-added `CharacterTrajectory` (`CharacterTrajectoryComponent`) and `ABP_UEFNSource_Player` caches that component in EventGraph.
- `GP_CharacterAnimInstance` now reflects that Blueprint `CharacterTrajectoryComponent` at runtime and copies its internal `Trajectory` into `GeneratedTrajectory`, falling back to `PoseSearchGenerateTransformTrajectory(...)` if unavailable.
- `GP_CharacterAnimInstance` tracks previous-frame locomotion context (`MovementMode_LastFrame`, `Gait_LastFrame`, `LastLocalVelocityDirection`, `LastVerticalVelocity`) so the stock UEFN chooser tables can be restored against named inputs instead of new ad-hoc branches.
- `ABP_UEFNSource_Player` still keeps the enum-blend fallback graph, but chooser-driven DB selection is being moved into a new custom root chooser for MaskMan.
- `/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/ChooserTables/CHT_MM_MaskMan_Root` is being authored with embedded `Idle`, `Run`, `Sprint`, and `InAir` nested choosers. `Walk` is intentionally omitted for now because MaskMan's default locomotion speed (`500`) should already use run-family PSDs.

## Project Snapshot

<!-- memoc:snapshot:start -->
- Last synced: 2026-05-20T09:04:23
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

See `.memoc/log.md` for full history.

## Commands

- Unreal Python: created `/Game/Characters/PlayerCharacter/ABP_UEFNSource_Player`
- Unreal BP edit: assigned `BP_GP_PlayerCharacter.UEFNSourceMesh.AnimClass = ABP_UEFNSource_Player`
- C++ edit: added `PoseSearch`/`Chooser` support, runtime trajectory bridging, runtime pose-search DB fallback, and chooser-facing context variables in `GP_CharacterAnimInstance`

## Notes

- Minimal source AnimGraph is `Motion Matching -> Pose History -> Output Pose`.
- `GeneratedTrajectory` is connected to `Pose History.TransformTrajectory` in `ABP_UEFNSource_Player`.
- `ABP_UEFNSource_Player` CDO now uses `PSD_Relaxed_Stand_Walk_F_Loops` for `WalkPoseSearchDatabase` instead of the broader `PSD_Relaxed_Stand_Walk_Loops`.
- `GP_CharacterAnimInstance` exposes `CurrentMotionMatchState` and still computes `RuntimePoseSearchDatabase` as state/debug data.
- `ABP_UEFNSource_Player` is still using enum-driven branches as a temporary fallback while the new chooser is authored and validated.
- Exported `/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/CHT_PoseSearchDatabases_Relaxed` to inspect required inputs. Root chooser expects `MovementMode`, `Stance`, `MovementState`, and `Gait`; nested tables also expect `MovementDirection`, `MovementDirection_Recent`, `MovementMode_LastFrame`, `Gait_LastFrame`, `IsStarting`, `IsPivoting`, `ShouldSpinTransition`, `JustTraversed`, `JustLanded_Light`, `JustLanded_Heavy`, `ShouldTurnInPlace`, and `Speed2D`.
- New custom chooser authoring started under `/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/ChooserTables/`; `Idle`, `Run`, `Sprint`, and `InAir` nested chooser rows were seeded from the stock relaxed chooser layout, but tuned around MaskMan's run-first locomotion (`500 -> Run`, `700 -> Sprint`).
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
