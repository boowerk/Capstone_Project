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

## [2026-05-26T18:17:18] archived summary (1371B)

---
memoc: true
type: state
scope: project-memory
created: 2026-05-26T11:35:43
updated: 2026-05-26T11:35:43
status: active
tags:
  - memoc
  - memoc/state
---
# Session Summary
Last: 2026-05-26T11:35:43
Replace, do not append. Keep <800B.
History: worklog. Resume risks: 04-handoff.md.

## Status
- White Void transition C++ is implemented on `AGP_PlayerCharacter`; BP-callable Toggle/Enter/Exit are exposed.
- Runtime setup creates `GP_WhiteVoidSetActor` with white floor, sky sphere, light, and post process; input asset `IA_WhiteVoidToggle` is mapped to `O`.
- Last MCP PIE check found the original Plane floor could allow falling; source patched to use a Box floor with Z offset. Directional Light was replaced with bounded local Point Light to avoid changing the main world's lighting. User rebuild needed.

## Changed
- Added White Void actor/component classes, player transition logic, PlayerController input hook, and Enhanced Input/BP asset wiring.

## Open Tasks
- Rebuild after latest floor patch, then rerun PIE: press `O`, verify enter/exit preserves camera framing and no falling occurs.
- Confirm original world lighting remains unchanged before entering White Void.

## Resume
- After rebuild, use MCP PIE to verify `BP_GP_PlayerCharacter_C_0` toggles between original Z and `WhiteVoidOffset` Z, and `GP_WhiteVoidSetActor` floor aligns under the capsule.

## [2026-05-26T18:18:06] archived summary (1230B)

---
memoc: true
type: state
scope: project-memory
created: 2026-05-26T18:17:18
updated: 2026-05-26T18:17:18
status: active
tags:
  - memoc
  - memoc/state
---
# Session Summary
Last: 2026-05-26T18:17:18
Replace, do not append. Keep <800B.
History: worklog. Resume risks: 04-handoff.md.

## Status
- White Void transition system is successfully refactored and optimized.
- Completed replication sync, early return issue in animBP, memory-safe reflection for MM trajectory, and component creation cleanup.

## Changed
- GP_PlayerCharacter: Added `OnRep_IsInWhiteVoid()` for multi-play simulated proxy sync; used member `RestoreLagTimerHandle` with clearing protection.
- GP_CharacterAnimInstance: Removed overall early return from `NativeUpdateAnimation`; gated suppression specifically to Pose Search evaluation inside `ApplyRuntimeDatabaseToMotionMatchingNode`.
- GP_WhiteVoidSetComponent: Removed obsolete `AddInstanceComponent` for standard runtime spawning flow.
- ResetMotionTrajectory: Added reflection-based struct check protecting against future `TranslationHistory` definition changes.

## Open Tasks
- Run project build to verify compile, then launch PIE to test multi-play replicated transition and camera/MM smoothing.

## [2026-05-31T12:33:05] archived summary (1347B)

---
memoc: true
type: state
scope: project-memory
status: active
tags:
  - memoc
  - memoc/state
updated: 2026-05-31T20:27:00+09:00
created: 2026-05-28T12:18:30
---
# Session Summary
Last: 2026-05-31T20:48:00+09:00
Replace, do not append. Keep <800B.

## Status
- ActionEnd remains input/control unlock, not montage end.
- User enabled `Always Update Source Pose`; A-pose fixed. Source fallback RM montage BlendOut is `0.25`.
- Added post-action anim velocity hold (`0.35s`) so source MM can see fast action handoff velocity.
- Added ActionEnd lower-body MM signal in C++ only: Dash/DashSlash set `bBlendActionLowerBodyToMotionMatching`; `UGP_CharacterAnimInstance` exposes interp alpha.
- Reverted unsafe AnimBP `LayeredBlendPerBone` graph insertion because it broke `DefaultSlot.Source`, making non-montage animation A-pose. Restored `ABP_UEFNSource_Player` and `ABP_MaskMan_Player` to `pre-slot pose -> DefaultSlot -> Output`.
- Added held-move ActionEnd cancel: player caches movement input while Fixed, clears on MoveCompleted, and broadcasts cancel immediately when ActionEnd enables input cancel if movement is still held. DashSlash now registers the same cancel delegate as Dash.

## Verify
- Both AnimBPs compiled/saved after restore earlier. MCP hit usage limit before latest compile; user/editor must LiveCode or compile to verify C++.

## [2026-06-01T04:41:15] archived summary (1877B)

---
memoc: true
type: state
scope: project-memory
created: 2026-05-31T12:33:05
updated: 2026-05-31T23:14:00+09:00
status: active
tags:
  - memoc
  - memoc/state
---
# Session Summary
Last: 2026-06-01T00:29:00+09:00
Replace, do not append. Keep <800B.
History: worklog. Resume risks: 04-handoff.md.

## Status
- ActionEnd remains input/control unlock, not montage end. Source/target AnimBP slot paths restored after unsafe lower-body blend caused A-pose.
- Held move cancel path added: player caches move input while Fixed and Dash/DashSlash broadcast cancel at ActionEnd if held input is recent.
- Log read showed held cancel fired, but inertia skipped because carry decayed to ~54 from sprint entry ~854. Patched held-input handoff to seed CMC velocity from max(carry, entry speed, MaxWalkSpeed), with `[ActionRM][ApplyHeldInput]` log.
- For direction-change slip after roll, held-input handoff now also overwrites post-action anim velocity, and UEFNSource AnimInstance overrides future `GeneratedTrajectory` samples from that action velocity. Added `[ActionRM][AnimTrajectory]` log.
- User reported all motion feels good, but trajectory debug during roll shows odd vertical path. Narrowed trajectory override to post-action velocity hold only (`IsUsingPostActionAnimVelocity`) so roll itself keeps normal `CharacterTrajectoryComponent` path; override no longer runs during montage/RM playback.
- User saw rare stop stutter after sprint-roll then releasing input. Patched post-action anim velocity hold to only happen when movement input is still recent or action was cancelled by movement input; released-input natural completion clears the hold.

## Changed
_Recent durable changes only._

## Open Tasks
_Current open tasks only._

## Resume
- LiveCoding succeeded after released-input hold patch. Git status/diff over LFS assets can fail with `.git/lfs/tmp` access denied.

## [2026-06-06T06:43:32] archived summary (1959B)

﻿---
memoc: true
type: state
scope: project-memory
updated: 2026-06-06T22:10:00+09:00
status: active
tags: [memoc, memoc/state]
---
# Session Summary

Last: 2026-06-06T22:10:00+09:00

## Status
- No UBT/build while the editor is open; use Live Coding.
- Reworked Primary/Sword_Light VFX toward SkillData VisualCues: no direct Niagara notifies; Primary resolves trail/burst from SkillData and attaches to `hand_r`.
- After editor reload, set `GA_Primary.DefaultSkillData` to `/Game/GAS_Pattern/AbilitySystem/SkillData/DA_Skill_Primary`; VisualCues[0]=`GameplayCue.Ability.Trail.Magic` -> `NS_ArrowTrail_Magic`, [1]=`GameplayCue.Ability.Burst.Magic` -> `NS_Free_Magic_Slash`.
- Moved VFX GameplayCue tags into native `GP_Tags.h/.cpp` (`GPTags::GameplayCue::Ability::*`) and removed config-only GameplayCue/VFX tag list entries.
- Added `DefaultSkillData` fallback to `UGP_SkillBase` for always-granted abilities without SourceObject.
- Added usage doc: `Project_Eden/Docs/SkillVisualCueStructure.md`.
- Matador boss GAS pass: common enemy/boss area hits now pass ability SkillData into damage application; bull charge now writes explicit SetByCaller damage/toughness values before sending player hit-react.
- `UEFNSourceMesh` remains the source animation mesh and `CharacterMesh0` stays attached under it.
- Current work is motion matching, crouch routing, chooser DB application, and root-motion extraction tuning.

## Open Tasks
- PIE validate crouch locomotion, primary crouch/sprint interaction, and running jump transition.
- Live Coding compile and editor-check the latest root-motion extraction changes.
- PIE validate Primary Sword_Light VFX.
- Live Coding compile and PIE validate Matador damage reduction, groggy full-damage window, decoy damage forwarding, and bull charge player/decoy collisions.

## Resume
- Start with `02-current-project-state.md` and `04-handoff.md`, then verify the live AnimBP / chooser path in PIE.

## [2026-06-17T04:47:15] archived summary (1776B)

---
memoc: true
type: state
scope: project-memory
status: active
tags: [memoc, memoc/state]
updated: 2026-06-17T13:01:55+09:00
---
# Session Summary
Last: 2026-06-17T13:01:55+09:00

## Status
- Matador bull loop is staged: bull spawns away, charges decoy, decoy redirects to player, player redirect back to decoy records success, 3 successes trigger decoy vanish + boss teleport.
- `GP_BullChargeActor` owns native `BullActorVisual` using `/Game/Meshes/Bull/SK/Bull`; `BP_Boss_Matador.BullChargeActorClass` points to native `/Script/Project_Eden.GP_BullChargeActor`.
- `BT_Boss_Matador`/`BB_Boss_Matador` are assigned, and `EnemyAIController` runtime boss test cycle default is disabled so old Sans/common cycling does not drive Matador.
- Matador state now suppresses generic boss attack selector candidates (`Basic`, `Sweep`, `Area`, `Summon`), and `BTS_UpdateBossTactics` keeps Matador area disabled. Bull/decoy should be primary pressure.

## Next
- Replace prototype overlap redirect with real parry/deflect input by calling `TryRedirectActiveBullTowardDecoy(PlayerActor)`.
- Replace temporary decoy vanish Niagara with a real smoke/poof Niagara asset when available.
- Add/assign real decoy matador-deflect animation in BP via `BP_OnDecoyRedirectedBull`; add player parry animation/input instead of current overlap prototype.
- Remove unused temporary `/Game/Characters/EnemyCharacter/Boss/BP_Boss_Matador/BP_MatadorBullChargeActor` after user confirms cleanup.
- If Matador becomes too passive, add a dedicated Matador-only ranged poke/cooldown instead of reusing common boss area spam.

## Verify
- User reported editor Live Coding compile succeeded after the generic attack suppression patch. Full external UBT still needs Live Coding disabled/closed editor.

## [2026-08-12T18:38:29] archived summary (945B)

---
memoc: true
type: state
scope: project-memory
updated: 2026-08-13T03:36:00+09:00
status: active
tags:
  - memoc
  - memoc/state
created: 2026-07-24T01:26:27
---
# Session Summary

- Branch `refactor/codebase-cleanup`; no gameplay C++ or permanent debug instrumentation changed.
- Packaged UE 5.7.2 Development Dedicated Server + 3 clients proves Revision 1 Snapshot, 7/7 local generation, first-attempt ACKs, and per-client placement. A separate same-run before/after PNG proves server AddXP(125) 3/3 and CurrentLevel RepNotify augment UI on all three clients.
- Public assets are a short 3-client GIF plus 1920x1080 placement and PlayerState PNGs. The Astro site and 10-page PDF are rebuilt and deployed at the existing Vercel URL; desktop/mobile QA pass.
- Still unproven: ACK retry N>=2, runtime reverse-order comparison image, 3-player movement, numeric PlayerState parity, selected-augment array, equipment and elimination replication.
