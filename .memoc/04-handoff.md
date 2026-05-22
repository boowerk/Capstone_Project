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
# Agent Handoff

Last synced: 2026-05-20T18:25:00

## What Changed

- Created `/Game/Characters/PlayerCharacter/ABP_UEFNSource_Player` for the UEFN mannequin skeleton.
- Assigned that AnimBP to `BP_GP_PlayerCharacter.UEFNSourceMesh`.
- Added trajectory generation support to `UGP_CharacterAnimInstance` using `UPoseSearchTrajectoryLibrary::PoseSearchGenerateTransformTrajectory`.
- Added runtime pose-search DB selection fields to `UGP_CharacterAnimInstance` and selected idle / walk / run / sprint / jump in `NativeUpdateAnimation()`.
- Rebuilt `ABP_UEFNSource_Player` AnimGraph around fixed idle / walk / run / sprint / jump source branches and `CurrentMotionMatchState` enum-driven selection.
- Added `CharacterTrajectory` component to `BP_GP_PlayerCharacter` and cached that component reference inside `ABP_UEFNSource_Player` EventGraph.
- Removed temporary `bUseIdle/Walk/Run/Sprint/JumpMotionMatch` helper flags from `UGP_CharacterAnimInstance`.
- Updated `UGP_CharacterAnimInstance` to prefer the Blueprint `CharacterTrajectoryComponent` at runtime by reflecting and copying its internal `Trajectory` into `GeneratedTrajectory`, while keeping the older generated trajectory path as fallback.
- Reconnected `ABP_UEFNSource_Player.GeneratedTrajectory` to `Pose History.TransformTrajectory`.
- Exported and inspected `CHT_PoseSearchDatabases_Relaxed`; confirmed the root chooser and nested tables are driven by named context values rather than mysterious hidden logic.
- Added chooser-facing context variables to `UGP_CharacterAnimInstance`: `MovementMode`, `MovementMode_LastFrame`, `Stance`, `MovementState`, `Gait`, `Gait_LastFrame`, `MovementDirection`, `MovementDirection_Recent`, `Speed2D`, `IsStarting`, `IsPivoting`, `ShouldSpinTransition`, `JustTraversed`, `JustLanded_Light`, `JustLanded_Heavy`, and `ShouldTurnInPlace`.
- Started a new custom chooser asset at `/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/ChooserTables/CHT_MM_MaskMan_Root` with embedded `Idle`, `Run`, `Sprint`, and `InAir` nested choosers.
- Seeded the new nested chooser rows from the stock relaxed chooser patterns, but tuned them for MaskMan's run-first locomotion (`500 -> Run`, `700 -> Sprint`) and preserved `TurnInPlace` in the idle cluster.
- Changed player movement to camera-facing/back-view style and added real directional movement speed scaling in `GP_PlayerController::Input_Move`.

## Next Steps

- Point `UGP_CharacterAnimInstance` (or the active AnimBP path) at `CHT_MM_MaskMan_Root` instead of the stock relaxed chooser fallback.
- Verify the new chooser in PIE: idle, turn-in-place, run, sprint, and in-air should all resolve to the intended PSDs.
- Verify actual movement speed in PIE: forward `500`, side `350`, back `300`, sprint forward `700`, sprint side/back `350/300`.
- Keep the enum/MM blend graph only as a temporary safety net; remove or simplify it after the new chooser proves stable.

## Blockers

- Directly injecting a new `AnimGraphNode_ChooserPlayer` into `ABP_UEFNSource_Player` through tooling caused an editor crash; avoid blind graph-node creation in the production AnimBP.
- `MovementDirection_Recent` was not reliably visible in the chooser UI during authoring. The first custom `Run` chooser pass may omit or duplicate that column until the variable is exposed cleanly.

## Do Not Touch Without Asking

_None yet._

## Verified

- `ABP_UEFNSource_Player` compiles and saves.
- `BP_GP_PlayerCharacter` CDO shows `UEFNSourceMesh.animClass = /Game/Characters/PlayerCharacter/ABP_UEFNSource_Player.ABP_UEFNSource_Player_C`.
- Build error about undefined `FTransformTrajectory` was fixed by including `Animation/TrajectoryTypes.h`.
- `ABP_UEFNSource_Player` compiles/saves with enum-driven branch selection.
- `ABP_UEFNSource_Player` CDO contains the expected idle / walk / run / sprint / jump pose-search database defaults.
- PIE runtime inspection showed locomotion state logic itself can resolve idle correctly (`GroundSpeed=0`, `bIsFalling=false`, `RuntimePoseSearchDatabase=PSD_Relaxed_Stand_Idles`) when checked in-session.
- Live Coding compile succeeded after adding direction-based controller movement scaling.
- `BP_GP_PlayerController` CDO shows `NormalForwardMoveSpeed=500`, `NormalSideMoveSpeed=350`, `NormalBackMoveSpeed=300`, `SprintForwardMoveSpeed=700`, `SprintSideMoveSpeed=350`, `SprintBackMoveSpeed=300`.
- `BP_GP_PlayerCharacter` CDO contains `CharacterTrajectory : CharacterTrajectoryComponent`.
- Export text of `CHT_PoseSearchDatabases_Relaxed` confirmed exact expected context names and that multiple nested chooser tables are in play (`Stand Idles`, `Stand Walks`, `Stand Runs`, `Stand Sprint`, `Crouch Idle`, `Crouch Moving`, `Slide`, `InAir`).
- `CHT_MM_MaskMan_Root` root rows were authored as:
  - `Idle -> Grounded / Idle / Any`
  - `Run -> Grounded / Moving / Run`
  - `Sprint -> Grounded / Moving / Sprint`
  - `InAir -> InAir / Any / Any`
- `Idle` nested chooser rows were authored for `Idles`, `TurnInPlace`, `Run_Stops`, `Sprint_Stops`, `Idle_Lands_Light`, and `Idle_Lands_Heavy`.
- `Run`, `Sprint`, and `InAir` nested chooser rows were seeded with the forward-loop/start/turn/pivot/land and jump-family PSDs discussed in-session.

## Not Verified

- PIE/runtime playback against the new `CHT_MM_MaskMan_Root`.
- Whether `MovementDirection_Recent` can be exposed cleanly enough to use distinctly from `MovementDirection` in the new `Run` chooser.

## Resume Notes

- `ABP_UEFNSource_Player` EventGraph already does `Try Get Pawn Owner -> Cast BP_GP_PlayerCharacter -> Get CharacterTrajectory -> Set Character Trajectory`.
- `GP_CharacterAnimInstance` now reflects `CharacterTrajectory` off the owning character class instead of requiring a fragile nested property-access node in the AnimGraph.
- `Blend Poses by Enum` node added through tooling could not set protected `BoundEnum`; user rebuilt enum blend manually in editor.
- Root chooser rows now intentionally ignore `Walk`; MaskMan's first-pass custom chooser is `Idle / Run / Sprint / InAir`.
- For custom chooser authoring, use embedded nested choosers (`New Nested Chooser`) inside `CHT_MM_MaskMan_Root`; `Select Existing` only sees embedded choosers in that asset.

## Suggested Reads

Search first, then open only files named above.
