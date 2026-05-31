# Session Summary Archive

Older oversized startup summaries moved by `memoc trim-summary`.

## [2026-05-21T07:03:24] archived summary (2345B)

# Session Summary
Last: 2026-05-20T18:25:00
Keep each section ≤ 3 bullets. Agent-owned — updated by you, not by `memoc update`.

## Status
- Runtime retarget path still uses `UEFNSourceMesh` as animation source and `CharacterMesh0` (`MaskMan`) as retarget target.
- `BP_GP_PlayerCharacter` has a Blueprint `CharacterTrajectoryComponent`, and `UGP_CharacterAnimInstance` now bridges that into `GeneratedTrajectory`.
- New custom chooser authoring started under `ChooserTables/CHT_MM_MaskMan_Root`, with `Idle / Run / Sprint / InAir` embedded nested choosers tuned for MaskMan's `500 -> Run`, `700 -> Sprint` locomotion.

## Changed
- Exported `/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/CHT_PoseSearchDatabases_Relaxed` and identified the root and nested chooser inputs.
- Added chooser-facing enums and variables to `UGP_CharacterAnimInstance`: `MovementMode`, `Stance`, `MovementState`, `Gait`, `MovementDirection`, `MovementDirection_Recent`, `Speed2D`, `MovementMode_LastFrame`, `Gait_LastFrame`, `IsStarting`, `IsPivoting`, `ShouldSpinTransition`, `JustTraversed`, `JustLanded_Light`, `JustLanded_Heavy`, `ShouldTurnInPlace`.
- Kept the current enum-branch AnimGraph alive as a temporary fallback while the chooser path is being restored safely, and fixed the user-found one-slot blend offset in `ABP_UEFNSource_Player`.
- Seeded `CHT_MM_MaskMan_Root` embedded chooser rows for `Idle` (`Idles`, `TurnInPlace`, `Run_Stops`, `Sprint_Stops`, `Idle_Lands_*`), `Run`, `Sprint`, and `InAir`.
- Switched the default runtime chooser load path to `CHT_MM_MaskMan_Root` and split sprint detection onto `SprintSpeedThreshold = 650` so base speed `500` remains run-family.

## Open Tasks
- Point runtime chooser evaluation at `CHT_MM_MaskMan_Root`.
- Verify in PIE that `Idle / TurnInPlace / Run / Sprint / InAir` resolve to the intended PSDs and that the half-step start issue improves.
- Decide whether `MovementDirection_Recent` needs separate exposure before expanding the new `Run` chooser beyond the initial forward-focused rows.

## Resume
- Start with `GP_CharacterAnimInstance.h/.cpp`; chooser input contract and runtime evaluation live there.
- Then finish wiring runtime to `/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/ChooserTables/CHT_MM_MaskMan_Root` and validate the new embedded nested choosers.

## [2026-05-22T19:34:58] archived summary (1084B)

---
memoc: true
type: state
scope: project-memory
created: 2026-05-21T07:03:24
updated: 2026-05-23T03:30:00
status: active
tags:
  - memoc
  - memoc/state
---
# Session Summary
Last: 2026-05-23T03:30:00

## Status
- Chooser Table mismatch bug identified: expects `ABP_UEFNSource_Player_C` but gets `ABP_MaskMan_Player_C`.
- Player rotation now targets semi-fixed Strafe/Aim feel: pawn yaw is not controller-locked; orient-to-movement stays off; controller desired rotation is enabled only during non-Fixed movement input.
- Directional/sprint `MaxWalkSpeed` changes now smooth in `GP_PlayerController` instead of snapping each input update.
- Turn In Place animation chattering fixed in C++ by implementing `TurnInPlaceMinDuration` (0.6s) hysteresis and MM asset name-based state lock.
- C++ modifications are fully implemented; waiting for compiler iteration in Unreal editor.

## Open Tasks
- Run Live Coding compile (Ctrl+Alt+F11) inside Unreal Editor to compile and hot-reload changes.
- Verify smooth Turn In Place rotations under PIE and verify no animation chattering remains.

## [2026-05-22T19:40:15] archived summary (1179B)

---
memoc: true
type: state
scope: project-memory
created: 2026-05-22T19:34:58
updated: 2026-05-22T19:34:58
status: active
tags:
  - memoc
  - memoc/state
---
# Session Summary
Last: 2026-05-22T19:34:58
Replace, do not append. Keep <800B.
History: worklog. Resume risks: 04-handoff.md.

## Status
- Turn In Place animation chattering resolved.
- Turning-but-not-fully-aligning camera problem resolved by tightening threshold (2.0 deg) and implementing a custom RInterpTo-based fine alignment block in GP_PlayerCharacter Tick.
- All code changes implemented. Ready for editor compile.

## Changed
- Added `TimeSinceTurnInPlaceStarted` and `TurnInPlaceMinDuration` to `GP_CharacterAnimInstance.h`.
- Updated `GP_CharacterAnimInstance.cpp` with turn-state hysteresis and MM selected animation lock.
- Added `#include "Animation/GP_CharacterAnimInstance.h"` and camera alignment `RInterpTo` block inside `GP_PlayerCharacter.cpp::Tick`.

## Open Tasks
- Run Live Coding compile (Ctrl+Alt+F11) inside Unreal Editor.
- Verify exact 1:1 camera-to-actor alignment during stationary mouse rotation.

## Resume
- Hot-reload active editor session and PIE test turn-in-place synchronization.

## [2026-05-22T19:45:06] archived summary (1101B)

---
memoc: true
type: state
scope: project-memory
created: 2026-05-22T19:40:15
updated: 2026-05-22T19:40:15
status: active
tags:
  - memoc
  - memoc/state
---
# Session Summary
Last: 2026-05-22T19:40:15
Replace, do not append. Keep <800B.
History: worklog. Resume risks: 04-handoff.md.

## Status
- Turn In Place animation chattering resolved.
- Resolved Turn In Place not aligning to camera by:
  (1) Updating missing `DesiredControllerYawLastUpdate` with `DesiredYaw` for correct MM trajectory prediction.
  (2) Disabling artificial `RInterpTo` rotation during active turn to prevent foot slippage (pure root motion).
  (3) Engaging a smooth `RInterpTo` lock (4.0 speed) only on residual post-turn error (< 10 deg) when turn-in-place is finished.

## Changed
- Updated `GP_CharacterAnimInstance.cpp` with `DesiredControllerYawLastUpdate` camera updates.
- Refined `GP_PlayerCharacter.cpp::Tick` fine alignment condition (`!bInTurnInPlace && YawDelta < 10.f`).

## Open Tasks
- Run Live Coding compile (Ctrl+Alt+F11) inside Unreal Editor.
- Verify 1:1 camera-to-actor alignment without foot sliding.

## [2026-05-22T19:55:28] archived summary (848B)

---
memoc: true
type: state
scope: project-memory
created: 2026-05-22T19:45:06
updated: 2026-05-22T19:50:00
status: active
tags:
  - memoc
  - memoc/state
---
# Session Summary
Last: 2026-05-22T19:56:00
Replace, do not append. Keep <800B.

## Status
- TurnInPlace alignment to camera Yaw completely resolved.
- Resolved initial foot sliding & chattering during 제자리 회전.

## Changed
- Refined GP_PlayerCharacter.cpp to accumulate RootMotionDeltaRot, with manual RInterpTo (5.0f) alignment delayed by 0.25s during TurnInPlace to preserve anticipation phase.
- Exposed public GetTimeSinceTurnInPlaceStarted() in GP_CharacterAnimInstance.h.
- Kept RInterpTo (3.0f) for residual yaw (<15 deg) post-turn for seamless alignment.

## Open Tasks
- Trigger Live Coding (Ctrl+Alt+F11) in Unreal Editor.
- Verify 1:1 camera-to-actor alignment in PIE.

## [2026-05-22T20:02:00] archived summary (891B)

---
memoc: true
type: state
scope: project-memory
created: 2026-05-22T19:55:28
updated: 2026-05-22T20:02:00
status: active
tags:
  - memoc
  - memoc/state
---
# Session Summary
Last: 2026-05-22T20:02:00
Replace, do not append. Keep <800B.

## Status
- TurnInPlace alignment to camera Yaw completely resolved.
- Eliminated 100% of foot sliding during micro-adjustments & early turn phases via Yaw Hysteresis & Late Snapping.

## Changed
- Refined GP_PlayerCharacter.cpp to completely lock actor rotation when not in TurnInPlace.
- Implemented late One-shot Snap to Camera (RInterpTo 12.0f after 0.4s into TurnInPlace), relying 100% on pure root motion during the first 0.4 seconds of the turn.
- Exposed public GetTimeSinceTurnInPlaceStarted() in GP_CharacterAnimInstance.h.

## Open Tasks
- Trigger Live Coding (Ctrl+Alt+F11) in Unreal Editor.
- Verify 1:1 camera-to-actor alignment in PIE.

## [2026-05-26T11:35:43] archived summary (3242B)

---
memoc: true
type: state
scope: project-memory
created: 2026-05-23T05:37:00
updated: 2026-05-24T00:35:00
status: active
tags:
  - memoc
  - memoc/state
---
# Session Summary
Last: 2026-05-26T20:12:00
Replace, do not append. Keep <800B.

## Status
- Added source-skeleton fallback montage slots to `PDA_CharacterAnimationSet`.
- `GP_PlayerCharacter` now applies UEFNSource fallback montage root-motion translation via `SafeMoveUpdatedComponent` and caches last RM velocity.
- Dash/Primary use PDA montage first, then UEFNSource fallback montage when empty.
- Fixed source fallback dash hang: `ABP_UEFNSource_Player` now has `DefaultSlot` between Pose History and Output Pose; `GP_Dash` adds a fallback-duration timer so missing ActionEnd notifies cannot leave `Fixed` stuck.
- Added PDA `SourceRootMotionTranslationYawOffset` default `-90` and applies it only to fallback source RM translation.
- Reverted bad experiments: no `UEFNSourceMeshScale` multiplication on consumed RM translation, and AnimInstance speed is back to `Character->GetVelocity()`.
- Reverted bad debug logging experiment; on-screen speed display is back to `GroundSpeed` / `Speed2D`.
- Fixed opposite-direction input linger: MoveAction Completed now resets input/smoothing separately, and move direction smoothing snaps on opposite direction.
- Found speed scale data issue: `PDA_MaskMan_AnimationSet.MovementSpeedProfile.MovementSpeedScaleRatio` was 1.0 while `UEFNSourceMeshScale` was 1.22. Runtime PIE asset/player were set to 1.22 and `UpdateAnimationSet` called; save is blocked during PIE.
- Speed debug now only prints from the locally controlled pawn's `CharacterMesh0`; stale transient worlds and `UEFNSourceMesh` cannot overwrite the same screen-debug key.
- `UGP_CharacterAnimInstance` now reads final `AGP_PlayerCharacter::GetMovementSpeedScaleRatio()` and `AGP_PlayerCharacter` pushes that ratio into both target/source anim instances when profiles or GAS scale change.
- Runtime PIE verify: active player `CharacterMesh0`/`UEFNSourceMesh` scale=1.22, raw=427, MM=350; stale `/Engine/Transient.World_0` remains player=false and is ignored by debug.
- `PoseSearchChooser` default load/evaluation is gated to the UEFNSource anim instance only; target `ABP_MaskMan_Player`/boss no longer feed the source-only chooser after Live Coding patch_5.
- Opposite-direction start acceleration clamp now skips when current velocity and acceleration oppose, reducing reversal linger.
- User confirmed left/right reversal improved after commit/build.
- Fallback UEFNSource root-motion montage translation now multiplies by `GetMovementSpeedScaleRatio()` after source yaw correction, restoring PDA movement scale effect without using visual `UEFNSourceMeshScale`.
- Fallback dash now reads `ActionEnd` timing from source montage notifies (`UGP_AnimNotify_SendGameplayEvent` or notify name) and schedules an ability-end timer at that point; full-duration timer remains as safety.

## Changed
- `PDA_CharacterAnimationSet.h`
- `GP_PlayerCharacter.{h,cpp}`
- `GP_Dash.{h,cpp}`
- `GP_Primary.cpp`
- `ABP_UEFNSource_Player.uasset`

## Open Tasks
- Re-test fallback dash roll ends at source montage `ActionEnd` timing.
- Live Coding compile succeeded.

## [2026-05-31T06:04:44] archived summary (737B)

---
memoc: true
type: state
scope: project-memory
created: 2026-05-26T11:35:43
updated: 2026-05-28T20:13:00
status: active
tags:
  - memoc
  - memoc/state
---
# Session Summary
Last: 2026-05-31T06:35:00
Replace, do not append. Keep <800B.
History: worklog. Resume risks: 04-handoff.md.

## Status
- Tech drives VFX/damage.
- SkillData has actor/VFX + `SkillIdTag`.
- PlayerState replicates augments; granted element applies tech.
- Matching augment `DamageMultiplier` scales base/base spell; user verified 2x damage.
- PulseBurst now applies matching augment `RadiusMultiplier` to overlap radius and VFX scale.

## Open Tasks
- Fill DA VFX/actor; connect range/projectile/cooldown modifiers.
- Later replace raw `K` with Enhanced Input.
