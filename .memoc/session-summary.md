---
memoc: true
type: state
scope: project-memory
updated: 2026-07-09T22:24:00+09:00
status: active
created: 2026-06-22T17:59:08
tags:
  - memoc
  - memoc/state
---
# Session Summary

## Status
- Added native `AGP_EncounterDebugDirector` for PIE encounter testing. It exposes BlueprintCallable controls for 3 stage slots, player teleport, mob/boss debug spawn, debug despawn, enemy list snapshots, selected enemy kill/destroy, HP percent set, AI enable/disable, loose tag add/remove/toggle, GameplayEffect apply, validation, and log broadcast.
- Added `FindActiveEncounterDebugDirector()` so Editor Utility Widgets can resolve the PIE/game-world director instead of accidentally calling the editor-world actor.
- Added native `UGP_EncounterDebugRuntimeWidget` and F9 hotkey on the placed director. This creates an in-game clickable debug panel with visible text buttons during PIE.
- Encounter debug selected-class spawning now uses one `Spawn Selected` button. Legacy `SpawnSelectedClassMob/Boss` calls remain but route to the same selected-class spawn path.
- Enemy class auto-discovery now filters AssetRegistry results through `AGP_EnemyCharacter` derived class names, so animation BPs like `ABP_FurnaceWalker_C`/`ABP_Enemy_Ranged_C` should no longer appear as spawnable enemy classes.
- Started Matador decoy pressure refactor. Added `UGP_MatadorDecoyPressureComponent` and attached it to `AGP_MatadorBossDecoyActor`; the component handles server-side player target selection, slow walking approach, out-of-range teleport reservation, teleport placement near a new target, combo index advancement on escape, and post-teleport attack lock state. It does not yet play the authored step-thrust montage or apply hitboxes.
- Wired existing Matador Rapier/Cape GAS events into the decoy actor: `AGP_MatadorBossDecoyActor` now exposes decoy-side Rapier/Cape BlueprintImplementableEvents plus `GetDecoyMesh()` and optional `DecoyAnimClass`. `UGP_MatadorMeleeAbilities` notifies the decoy pressure component while Rapier/Cape are active so teleport requests wait until the current action ends.
- Corrected Matador decoy pressure feel: this is not distance keeping. The decoy walks in until skill staging range, treats escape only past `MeleeExitRange=1600` for `2.25s`, keeps the current target when executing reserved teleports unless that target is invalid, and teleports near that same target at skill-range pressure distances. Decoy decorative static meshes are forced `NoCollision` at layout time so players can approach the decoy.
- Fixed the remaining "decoy moves away" cause: legacy `AGP_MatadorMageBossCharacter::UpdateDecoyFollow()` was still moving the active decoy toward a 650cm desired distance every tick. It now skips that legacy distance-keeping follow while the decoy pressure component is active.
- Retuned Matador decoy pressure to skill range instead of body-hugging: `MeleeEnterRange=700`, `MeleeExitRange=1600`, `TargetOutOfRangeTime=2.25`, teleport distances `700/850/1000`. The decoy approaches until skill staging range, but does not retreat if the player moves closer.
- Clarified Matador decoy mesh ownership: because the actor inherits `AGP_EnemyCharacter`, it still has inherited `CharacterMesh0`, but gameplay presentation uses `DecoyMesh`. `ApplyDefaultVisualLayout()` now force-hides/disables collision and overlaps on inherited `GetMesh()` every construction/begin-play pass so only `DecoyMesh` is visible/interactive.
- Added native `UGP_MatadorDecoyAnimInstance`, derived from `UGP_CharacterAnimInstance`, to keep existing locomotion graph variables while exposing decoy pressure state, target, teleport lock, walking-pressure state, and step-thrust index to `ABP_MatadorDecoy`. `ABP_MatadorDecoy` was reparented/saved to this class by editor commandlet after build.
- Corrected Matador decoy animation event handling to be ABP-driven, not montage-driven. `UGP_MatadorDecoyAnimInstance` now exposes `PresentationState`, direction, elapsed/duration, Rapier/Cape booleans, and cape burst indices. `AGP_MatadorBossDecoyActor` forwards Rapier/Cape ability events into these anim-instance state variables and no longer plays decoy montages directly.
- Clarified decoy animation ownership: locomotion remains ABP/AnimSequence-driven, while Rapier/Cape attack montages are now owned and played by `UGP_MatadorDecoyAnimInstance` through the ABP `DefaultSlot` instead of direct actor mesh playback.
- Wired `ABP_MatadorDecoy` locomotion: its final pose path is now Idle `MM_Matador_M_Relaxed_Stand_Idle_Loop` / Walk `MM_Matador_M_Neutral_Walk_Loop_F` blended by `bIsWalkingPressure`, then `DefaultSlot`, then `Output Pose`. The old retarget-from-mesh branch remains disconnected from the final output.
- Added `docs/MatadorDecoy_ABP_and_EnemyLeashDesign.md` as an editor-facing design guide for hand-authored Matador decoy ABP states and a more Soulslike enemy leash-return flow with Warning/GiveUp/TurnTowardHome/ReturnHome/Recenter phases.
- Extended that design guide with the planned transition-bool architecture: ABP transition rules should connect to simple `bCan_StateA_To_StateB` booleans computed in C++/AnimInstance, and skills should use PreRoll/ReadyHold/MontageActive/Recover states so montages do not pop directly from locomotion.
- Fixed Matador Bull pattern starvation: recent tuning had narrowed Bull range to `900-2600`, reduced the selection window to `0.8s/10s`, and lowered Bull score below Rapier/Cape, so Bull rarely or never surfaced. Restored Bull range defaults to `0-5000`, widened Bull windows to `2.5s`, and raised Bull selector priority as a core chain-break mechanic.
- Extended the in-PIE F9 encounter debug panel with a live selected-enemy report. It refreshes every 0.1s and shows selected enemy identity, HP, distance, movement, AI/Blackboard mirrors, predicted next boss skill/candidates from the shared selector, GAS tags/active abilities/granted abilities, Matador boss state, and Matador decoy pressure/ABP presentation state.
- Fixed Matador decoy teleport placement using navmesh floor positions as character roots. The pressure component now lifts projected teleport locations by the owner's capsule half height so the decoy capsule root is above the floor instead of spawning the visible mesh underground.
- Reorganized the F9 selected-enemy report UI into mode buttons: `Sum`, `AI`, `Skill`, `GAS`, `Mat`, and `All`. Default `Sum` is compact for in-combat reading; detailed views remain available without filling the whole panel.
- Reworked the F9 selected-enemy monitor from one scroll text block into dashboard cards: `Selected`, `Next Skill`, `Linked Actors`, and a smaller `Detail` pane. The linked-actor card is always visible and shows Matador boss/decoy/bull relationships plus decoy pressure target/teleport/step state when applicable.
- Intended EUW shape: `EUW_EncounterDebugPanel` should stay thin and call the placed BP child of this director.
- Project build rule corrected: Codex should close/confirm Unreal Editor is closed before C++ builds, then use the generated Rider/UE project-file engine path for this project: `D:\Engine\Windows\Engine\Build\BatchFiles\Build.bat`.
- `McpAutomationBridge` project settings were restored: plugin remains enabled, external `AdditionalPluginDirectories` remains present, and Native MCP loading remains enabled.
- Existing user/editor map change remains unowned: `Project_Eden/Content/Maps/EventMap/EventMap.umap`.
- Region Event examples now include direct PIE trigger BPs under `/Game/RegionEvents/Examples`: `BP_RE_TestTrigger_RedRift`, `BP_RE_TestTrigger_CrystalCorruption`, `BP_RE_TestTrigger_ShrineRuins`, and `BP_RE_TestTrigger_StructureDefense`.
- Drop one trigger BP into a level and PIE starts that single event without the GameMode/director zone flow.

## Verified
- MCP confirmed PIE `EventMap` contains `BP_EncounterDebugDirector_C_1` and player pawn, but no `EncounterDebugSpawned` enemies existed.
- Latest build succeeded with `D:\Engine\Windows\Engine\Build\BatchFiles\Build.bat Project_EdenEditor Win64 Development -Project="D:\Unreal Projects\Capstone_Project\Project_Eden\Project_Eden.uproject" -WaitMutex -FromMsBuild -architecture=x64`; Unreal Editor was closed first, and it compiled the Matador decoy pressure/event bridge changes and linked `UnrealEditor-Project_Eden.dll`.
- Latest side build also succeeded with the same `D:\Engine\Windows\Engine\Build\BatchFiles\Build.bat ... -FromMsBuild` command after closing Unreal Editor; it compiled `GP_MatadorDecoyAnimInstance` inheriting from `GP_CharacterAnimInstance` and the pressure/collision tuning changes. A follow-up editor commandlet successfully reparented/saved `ABP_MatadorDecoy` to `GP_MatadorDecoyAnimInstance`.
- Follow-up build succeeded after disabling legacy decoy distance-keeping while pressure is active.
- Follow-up build succeeded after skill-range pressure retune; editor commandlet saved the same pressure defaults onto `BP_MatadorDecoy`.
- Follow-up build succeeded after forcing inherited decoy `CharacterMesh0` hidden/no-collision while keeping `DecoyMesh` as the visible presentation mesh.
- Follow-up build succeeded after removing the direct decoy montage fallback and switching Rapier/Cape presentation to ABP-readable state variables.
- Follow-up build succeeded after moving Rapier/Cape montage playback into `UGP_MatadorDecoyAnimInstance`; build used `D:\Engine\Windows\Engine\Build\BatchFiles\Build.bat ... -FromMsBuild` after closing Unreal Editor.
- MCP compile/save of `ABP_MatadorDecoy` succeeded after wiring Idle/Walk locomotion into `DefaultSlot -> Output Pose`; no C++ rebuild was needed for this asset-only change.
- Follow-up build succeeded after restoring Bull pattern selection/range/window defaults; build used `D:\Engine\Windows\Engine\Build\BatchFiles\Build.bat ... -FromMsBuild` after closing Unreal Editor.
- Follow-up build succeeded after adding the F9 selected-enemy live report; Unreal Editor was closed first and build used `D:\Engine\Windows\Engine\Build\BatchFiles\Build.bat Project_EdenEditor Win64 Development -Project="D:\Unreal Projects\Capstone_Project\Project_Eden\Project_Eden.uproject" -WaitMutex -FromMsBuild -architecture=x64`.
- Follow-up build succeeded after fixing decoy teleport Z placement and reorganizing the F9 report UI into category modes; build used the same `D:\Engine\Windows\Engine\Build\BatchFiles\Build.bat ... -FromMsBuild` command with Unreal Editor closed.
- Follow-up build succeeded after reworking the F9 monitor into dashboard cards for selected enemy, next skill, linked actors/summons, and detail tabs.
- `Project_EdenEditor Win64 Development` build succeeded.
- `ProjectEden.Game.RegionEvents.ExampleAssets` and `ProjectEden.Game.RegionEvents.Selection` passed.

## Handoff
- Reopen Unreal Editor, PIE the level, press F9 to toggle the runtime encounter debug panel. Buttons should be visible and clickable in-game.
- Select a spawned monster in the F9 enemy combo. Expected: the fixed-height scroll area updates live with Blackboard/GAS/predicted skill information; for Matador, Bull/Rapier/Cape candidate scores and bull readiness flags should be visible.
- In the F9 panel, the class combo should list spawnable `AGP_EnemyCharacter` BP classes only; if animation BP names still appear, refresh/reopen PIE and check AssetRegistry derived-class filtering.
- Next Matador step: PIE-check `ABP_MatadorDecoy` on spawned decoys. Expected: idle pose while stopped, walk loop while pressure-walking, Rapier/Cape montages layered through DefaultSlot on `DecoyMesh`. Add a dedicated step-thrust ability/montage bridge with AnimNotify hit windows for normal and strong thrust.
