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

Last synced: 2026-05-26T20:45:00

## Current Tech UI Handoff

- Tech selection UI test path exists: `GP_TechSelectWidget` C++ parent + `WBP_TestTechSelect` child.
- Widget buttons must keep exact names for auto-bind: `Button_Pyros`, `Button_Hydro`, `Button_Volt`, `Button_Aero`, `Button_Lux`, `Button_Chaos`, `Button_Brute`.
- User made test PlayerController toggle widget with raw keyboard `K`; this works for now.
- Later production pass: replace raw `K` event with Enhanced Input action `IA_ToggleTechSelect` and map it in the player input context.
- Keep test PlayerController/GameMode separate from main BP to reduce team merge conflicts until feature is stable.

## Projectile VFX Handoff

- `UGP_SkillData` now has `SpawnActorClass`. Projectile-style GAs prefer this value and fallback to their old BP class variable when empty.
- `FGP_ElementVisualActorEntry` now has `ProjectileVisualSystem` for one Niagara system per element entry.
- `AGP_Projectile` and `AGP_AreaProjectile` replicate this system and call `BP_OnProjectileVisualSystemChanged`.
- Projectile BPs must implement that BP event and set their Niagara component asset from the event parameter.
- DA entries with no projectile VFX keep the existing BP/default visual because null visual systems are ignored.
- `ThrownBurst`, `MineBurst`, `NetTestProjectile`, and `SplitShot` read spawn actors from SkillData first.
- `ThrownBurst`, `NetTestProjectile`, and `SplitShot` pass projectile VFX from SkillData; `MineBurst` has no projectile VFX use yet.

## Skill Augment Handoff

- `UGP_SkillAugmentData` was added as a data-only shell.
- Skill identity tags exist under `GPTags.Ability.Skill.Id.*`; `UGP_SkillData.SkillIdTag` should be filled per DA in editor.
- `UGP_SkillAugmentData.TargetSkillTags` now filters to `GPTags.Ability.Skill.Id.*`.
- Fields cover display info, `TargetSkillTags`, optional `GrantedElementTag`, numeric modifiers, `ImpactVisualActorOverride`, and `ActiveVFXOverride`.
- `AGP_PlayerState` stores replicated `SelectedSkillAugments` and exposes `AddSkillAugment` / `ServerAddSkillAugment`.
- Adding an augment with `GrantedElementTag` currently updates `CurrentTechElementTag`, so element augments can reuse the existing skill VFX/damage element path.
- No runtime system reads numeric modifiers yet. Next pass should decide how modifiers merge into SkillData damage/cooldown/radius/projectile count.

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
- Moved directional speed values into `PDA_CharacterAnimationSet.MovementSpeedProfile`; player copies the PDA profile and applies GAS-ready runtime multipliers/overrides.
- Removed legacy PDA animation slots for locomotion blendspace, sprint stop, jump loop, landing montage, and sprint enter/exit montages because runtime motion matching now owns those paths.
- Added source fallback montage path for player Dash/Primary: target PDA montages still win, empty slots can play source montages on `UEFNSourceMesh`, and fallback root-motion translation is applied to the capsule via `SafeMoveUpdatedComponent`.
- Fixed the first source fallback dash test issue: `ABP_UEFNSource_Player` now has `DefaultSlot` between Pose History and Output Pose, and `GP_Dash` has a fallback-duration timer so missing `ActionEnd` notifies do not leave the ability's `Fixed` tag stuck.
- Added `PDA_CharacterAnimationSet.SourceRootMotionTranslationYawOffset` default `-90` to correct UEFN source fallback root-motion translation axis without rotating the montage pose or RM rotation.
- Reverted the attempted `UEFNSourceMeshScale` multiplier on consumed fallback root-motion translation; it made runtime movement/debug speed worse.
- Reverted the attempted `GP_CharacterAnimInstance` fallback-root-motion velocity speed override; speed/debug context is back to `Character->GetVelocity()`.
- Reverted the actor-location-delta debug speed display because it jittered while idle; on-screen speed display is back to `GroundSpeed` / `Speed2D`.
- Fixed opposite-direction input linger in `GP_PlayerController`: MoveAction `Completed` now resets input/smoothing separately, and movement direction smoothing snaps when the raw direction opposes the smoothed direction.
- Speed debug display now only prints from `CharacterMesh0`'s anim instance (`GetSkelMeshComponent() == Character->GetMesh()`), preventing `UEFNSourceMesh` from overwriting the same debug keys.
- Move smoothing now snaps when the desired direction differs strongly (`dot < 0.5`), not only for exact opposite vectors.

## Next Steps

- Point `UGP_CharacterAnimInstance` (or the active AnimBP path) at `CHT_MM_MaskMan_Root` instead of the stock relaxed chooser fallback.
- Verify the new chooser in PIE: idle, turn-in-place, run, sprint, and in-air should all resolve to the intended PSDs.
- Verify actual movement speed in PIE: forward `500`, side `350`, back `300`, sprint forward `700`, sprint side/back `350/300`.
- Verify `PDA_MaskMan_AnimationSet.MovementSpeedProfile` after exiting PIE; saving the asset was blocked while Play Mode was active.
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
- Controller no longer owns directional speed properties; it asks `AGP_PlayerCharacter::ResolveDirectionalMoveSpeed(...)`.
- Live Coding compile succeeded after adding `FGPDirectionalMovementSpeedProfile` and moving speed ownership from controller to player/PDA.
- `BP_GP_PlayerCharacter` CDO contains `CharacterTrajectory : CharacterTrajectoryComponent`.
- `ABP_UEFNSource_Player` compiles/saves with `Pose History -> DefaultSlot -> Output Pose`.
- UHT passed after adding the dash fallback timer; full UBT still cannot run while the editor's Live Coding session is active.
- Live Coding compile succeeded after adding source root-motion translation yaw offset.
- Live Coding compile succeeded after reverting the root-motion scale multiplier and AnimInstance speed override.
- Live Coding compile succeeded after reverting debug logging and fixing opposite-direction input smoothing.
- Full Project_EdenEditor Development Win64 build succeeded after the debug-owner gate and smoothing threshold change.
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
- PIE validation of `SourceRootMotionTranslationYawOffset = -90` on `AM_UEFN_Roll_RM`; if movement flips left instead, set the PDA value to `90`, and if the asset is already authored in UE forward axis set it to `0`.

## Resume Notes

- `ABP_UEFNSource_Player` EventGraph already does `Try Get Pawn Owner -> Cast BP_GP_PlayerCharacter -> Get CharacterTrajectory -> Set Character Trajectory`.
- `GP_CharacterAnimInstance` now reflects `CharacterTrajectory` off the owning character class instead of requiring a fragile nested property-access node in the AnimGraph.
- `Blend Poses by Enum` node added through tooling could not set protected `BoundEnum`; user rebuilt enum blend manually in editor.
- Root chooser rows now intentionally ignore `Walk`; MaskMan's first-pass custom chooser is `Idle / Run / Sprint / InAir`.
- For custom chooser authoring, use embedded nested choosers (`New Nested Chooser`) inside `CHT_MM_MaskMan_Root`; `Select Existing` only sees embedded choosers in that asset.

## Suggested Reads

Search first, then open only files named above.
