---
memoc: true
type: state
scope: project-memory
created: 2026-05-21T07:03:24
updated: 2026-06-19T08:31:17+09:00
status: active
tags:
  - memoc
  - memoc/state
---
# Agent Handoff

Last synced: 2026-06-19T08:31:17+09:00

## Enemy Leash Handoff

- Commits `aa0522f5` and `971d8f29` fix active enemies abruptly ignoring the player at `ReturnHomeDistance`. Full editor build and `ProjectEden.AI.Enemy.LeashPolicy` passed; PIE-check chase beyond the old limit, target loss outside the limit, and re-engagement during return.

## Enemy Health Bar Handoff

- Commits `6c668c6b` and `130afeb2` attach `WBP_EnemyHealthBar` to the shared enemy parent at Z=135, keep it visible at full health, bind GAS Health/MaxHealth, hide it for bosses/dead enemies, and add a passing defaults test. Editor build passed; PIE visual placement still needs checking on differently sized meshes.

## Enemy Death Handoff

- Commits `4783ba61` and `a0ae0cb4` implement shared HP-zero death for melee, ranged, flying, and boss children of `AGP_EnemyCharacter`. Editor build and `ProjectEden.Combat.EnemyDeath.Lifecycle` passed.
- PIE-check representative enemy/Boss assets for lethal damage and the 2-second default despawn. No editor assignment is required; later animation work should implement `BP_OnDeathStarted` and tune `DeathDespawnDelay`.

## FurnaceWalker Handoff

- `ABP_FurnaceWalker`: stationary uses full-body `DefaultSlot`; movement layers its attack pose from `spine_01` over locomotion. Existing Idle/Jog pin order is user-corrected; do not rewire it blindly. ABP changes remain uncompiled/unsaved by request.
- `UPDA_EnemyAnimationSet` is newly added but not built. `AGP_EnemyCharacter` exposes `EnemyAnimationSet` and applies its mesh/AnimBP; `UGP_EnemyAttack` reads its attacks, falling back to legacy `PDA_CharacterAnimationSet` for unmigrated bosses. After external build, create and assign `PDA_FW_EnemyAnimationSet`, then clear the legacy `AnimationSet` on `BP_FurnaceWalker`.
- FurnaceWalker target attacks: Mutant Punch L/R and Zombie Smashing L/R. Each selected montage has direct `AttackHit`/`ActionEnd` events; all use 0.25s Blend Out. Smashing events: 1.63s/3.45s.
- No MCP build: use the user's correct engine/UBT workflow, then PIE-check mesh/ABP assignment, random alternating attacks, event damage/release, and upper-body movement attacks.

## Sans Ground Hands Handoff

- Commit `e080f16c` updates `UGP_BossSummonAdds` to spawn `Basic/BP_BasicEnemy_Melee`; full editor build passed and startup logs showed no class-finder failure for the new path.
- Commits `f4e75d83` and `8a7fbc60` add the native strike actor plus `Attack_BossGroundHands` GAS ability/selector/default grant. Defaults are 3 hands per wave, 3 waves, 0.22s hand stagger, 1.55s wave interval, 0.75s red decal warning, and 8s ability cooldown.
- Full `Project_EdenEditor Win64 Development` build and `ProjectEden.AI.Boss.PatternSelector.SansGroundHands` passed. PIE still needs visual timing, decal projection, hit collision, damage, and vertical launch/fall verification.
- The broader `ScoreCases` test still has an unrelated existing Matador expectation mismatch (Cape expected, Rapier selected); the Sans-specific test is isolated and green.

## Basic Ranged Enemy Handoff

- Commits `573e1707`, `ab0171f4`, and `d526e4f9` add an overridable shared hit point and a native player-aimed projectile for `BP_BasicEnemy_Ranged` while retaining `BT_EnemyCommon` patrol/chase/attack flow. Aim is recalculated from the elevated spawn point to the player capsule center.
- Full `Project_EdenEditor Win64 Development` build passed. PIE-check the 850 cm attack band, trajectory, and damage.

## Crystal Seraph Boss Handoff

- Commits `1c9cd768`, `759988f2`, and `e9616312` throttle pattern starts, restore prism/laser rotation, and force `BT_BossCommon`/`BB_BossCommon`. Editor build and the Crystal Seraph selector automation test passed.
- PIE still needs runtime confirmation that attacks are at least 2.5s apart and the shared patrol/chase/reposition branches behave acceptably with `MOVE_Flying`.
- Native prototype is complete and builds: use `AGP_CrystalSeraphBossCharacter` as the C++ parent for a BP child. The constructor assigns `/Game/Characters/MaskMan/SK_MaskMan` as the requested prototype mesh.
- Add these optional Blackboard keys to the Crystal Seraph BB asset if using BT scoring: `WingCoreBreakCount` int, `bCanExposeWingCore` bool, `bWingCoreExposed` bool, `CrystalPrismActor` object, `bCanUseLaserPattern` bool, and `bCanUsePrismPattern` bool. Existing shared keys `bIsGroggy`, `PreferredHoverHeight`, `PreferredAirRange`, and `bShouldTeleport` are reused.
- Prototype visuals use engine basic shapes. Create BP children for `GP_CrystalPrismActor`, `GP_SeraphLaserActor`, `GP_WingCoreHitActor`, `GP_CrystalShardProjectile`, and `GP_CrystalSanctuaryMarkerActor` to assign final meshes/materials/Niagara.
- Damage rules are tag-driven in `GP_DamageExecCalculation`: `CrystalGuarded` = 15% final damage, `WingCoreExposed` = 50%, `Groggy` = full damage. PIE verify with `gp.DamageExec.Log 1`.
- `UBTT_ExecuteBossAttack` treats failed GAS activation as a real failure now: if a granted pattern cannot activate because of cooldown/block tags/state, it logs the failure, tries the next scored candidate, and returns Failed when no candidate activates. If Attack still loops without a pattern, inspect those `[BossAI] Pattern activation failed...` logs for the exact blocked tag/candidate.
- `BT_BossCommon.uasset` was observed still referencing `BTT_ExecuteEnemyAttack` in the Attack branch. C++ now guards this by routing boss pawns through `BossAttackExecution` even from the generic task. The editor-side clean setup is still to replace that Attack node with `BTT_ExecuteBossAttack` when convenient.
- Build verification succeeded, but PIE validation still needs editor setup: BP child, BT/BB asset wiring, arena placement, and pattern VFX tuning.

## Skill Augment Handoff

- `UGP_SkillAugmentData.TargetSkillTags` empty means global/all-skill augment; otherwise exact skill id match.
- `UGP_SkillAugmentData.RequiredElementTag` gates modifier application by `AGP_PlayerState.CurrentTechElementTag`; empty means no element requirement.
- `GrantedElementTag` remains the "change current tech element" field and is separate from `RequiredElementTag`.
- `AGP_PlayerState` exposes damage, radius, range, cooldown multiplier, and projectile count bonus resolution.
- `AGP_PlayerState` now resolves `ActiveVFXOverride` and `ImpactVisualActorOverride`; `UGP_SkillBase` applies the latest matching selected augment override before SkillData element/default visuals.
- Next PIE test: create a Pyros-only projectile augment targeting NetTestProjectile, assign `NS_Free_Magic_Projectile1` to `ActiveVFXOverride`, select it, and verify the projectile Niagara changes.
- `DamageMultiplier` is passed as `Damage.Multiplier` and applied once to computed base damage before crit/increase/mitigation. Verify NetTestProjectile damage changes from 10 to 15 with a 1.5 augment.
- `UGP_SkillBase::ApplyCooldown` applies `CooldownMultiplier`; `UGP_Skill_DashSlash` also applies it to its fallback cooldown.
- `ProjectileCountBonus` is wired for `SplitShot`, `NetTestProjectile`, and `ThrownBurst`; NetTest/ThrownBurst fan extra projectiles across a small 10-degree yaw spread.
- PIE test still needed: cooldown DA (`CooldownMultiplier=0.5`), projectile DA (`ProjectileCountBonus=1`), and element-gated DA (`RequiredElementTag=Volt`) via augment picker.
- `AGP_PlayerState::GetSkillAugmentRadiusMultiplier` returns multiplied radius modifiers for selected augments exact-matching `SkillData.SkillIdTag`.
- `UGP_SkillBase` exposes `GetSkillAugmentRadiusMultiplier`, `GetSkillSpawnActorClass`, and `GetProjectileVisualSystem`.
- PulseBurst/GroundBurst apply radius multiplier directly to overlap radius.
- PulseBurst/GroundBurst pass the same multiplier to spawned visual actor scale.
- MineBurst/ThrownBurst apply radius multiplier to spawned `AGP_MineBurstActor` / `AGP_AreaProjectile` via `ApplyExplosionRadiusMultiplier`; impact visual actor scale uses the same multiplier.
- `AGP_PlayerState::GetSkillAugmentRangeMultiplier` and `UGP_SkillBase::GetSkillAugmentRangeMultiplier` are wired.
- RangeMultiplier scales LineShock box length, ConeSlash candidate range/visual offset, GroundBurst target forward offset, and MineBurst place forward offset.
- Verify in editor after compile: enable relevant debug radius and test matching augment DA per skill id.

## Current Tech UI Handoff

- Boss HUD fix in progress: `GP_PlayerHUDWidget` now binds boss ASC via `UGP_AttributeWidget` (`BossBar`/`BossHealthBar` name lookup) and `GP_PlayerController::RefreshBossHUD()` re-calls binding for the current boss so live/widget swaps can recover.
- User created `/Game/UI/HUD/WBP_BossBar` from `UGP_AttributeWidget`; it was configured to Health/MaxHealth and placed as `WBP_PlayerHUDWidget.BossBar`. If editor Blueprint compile reports a stale `BossBar_ProgressBar_Deprecated` GUID ensure, recreate/delete that designer widget manually rather than continuing Python mutation.
- Tech selection UI test path exists: `GP_TechSelectWidget` C++ parent + `WBP_TestTechSelect` child.
- Widget buttons must keep exact names for auto-bind: `Button_Pyros`, `Button_Hydro`, `Button_Volt`, `Button_Aero`, `Button_Lux`, `Button_Chaos`, `Button_Brute`.
- User made test PlayerController toggle widget with raw keyboard `K`; this works for now.
- Later production pass: replace raw `K` event with Enhanced Input action `IA_ToggleTechSelect` and map it in the player input context.
- Keep test PlayerController/GameMode separate from main BP to reduce team merge conflicts until feature is stable.

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

- For PLAZA_DE_TOROS runtime construction: close editor/game or press Ctrl+Alt+F11 to disable active Live Coding, then rebuild. Place `AGP_LevelBuildAnimator` in the arena, set `TargetActorTag=PLAZA_DE_TOROS`, tag all controlled structure actors with `PLAZA_DE_TOROS`, and tune `PieceDuration`, `PieceOverlapDelay`, `UndergroundOffset`, `MirrorSpiralRadius`, `StartYawTwist`, `RiseOvershoot`, and order mode in Details.
- Build and PIE-test the corrected latest `feature/vfx-skills-impact` merge: Primary attack VisualCues, augment VFX priority, Matador AI/BT, and map asset loads.
- Future skill UI requirement: when an upgrade augment transforms a skill, replace the skill-slot presentation so it appears evolved. Add augment presentation overrides such as `SkillNameOverride`, `SkillDescriptionOverride`, and `SkillIconOverride`, then resolve the latest applicable selected augment before base `UGP_SkillData` display fields.
- Point `UGP_CharacterAnimInstance` (or the active AnimBP path) at `CHT_MM_MaskMan_Root` instead of the stock relaxed chooser fallback.
- Verify the new chooser in PIE: idle, turn-in-place, run, sprint, and in-air should all resolve to the intended PSDs.
- Verify actual movement speed in PIE: forward `500`, side `350`, back `300`, sprint forward `700`, sprint side/back `350/300`.
- Verify `PDA_MaskMan_AnimationSet.MovementSpeedProfile` after exiting PIE; saving the asset was blocked while Play Mode was active.
- Keep the enum/MM blend graph only as a temporary safety net; remove or simplify it after the new chooser proves stable.

## Blockers

- Full C++ build for `AGP_LevelBuildAnimator` used the correct `D:\Engine\Windows\Engine\Build\BatchFiles\Build.bat` path and real `.uproject`; UHT passed, but compile stopped with `Unable to build while Live Coding is active`. Need editor/game exit or Ctrl+Alt+F11 before rebuild.
- Latest Live Coding attempt after the final boss-binding idempotency edit is stuck in UBA `Low on memory` retry logs, not a C++ syntax error. Per user preference, ask for editor rebuild/restart instead of forcing another UBT build.
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

- 2026-06-17 basic enemy templates: C++ build succeeded and BP templates were created, but PIE runtime behavior still needs checking for common BT chase/attack transitions, ranged hit distance, and flying movement/pathing.
- 2026-06-17 UE Python commandlet created the Basic enemy BP assets successfully but returned failure because existing `Content/Maps/DemoMap/TestMap.umap` is unloadable (`Invalid value for PACKAGE_FILE_TAG`).
- 2026-06-14 generic boss Attack task routing: `Project_EdenEditor Win64 Development` build succeeded, but PIE still needs checking. Expected log from a stale generic Attack node is `[BossAI] Generic attack task routed through boss pattern selector...`.
- 2026-05-31 `UGP_SkillAugmentPoolData`: compile, create DataAsset, fill `Augments`, call `PickRandomAugments(3)` from BP, and pass result to `UGP_AugmentSelectWidget::SetCandidateAugments`.
- 2026-06-01 `UGP_AugmentSelectWidget`: verify BP child binds `TextBlock_Description0..2` and `Image_Icon0..2`; candidate cards should show augment name, description, and icon when DA fields are set.
- 2026-06-02 augment type backgrounds: full UBT/editor rebuild was not run because Unreal MCP build/query calls were rejected by automatic approval review. After rebuild, verify `AugmentTypeBackgrounds` defaults and `Image_CardBg0..2` bindings in `WBP_TestAugmentSelect_1`.
- 2026-05-30 DashSlash skill PIE/runtime behavior: verify `GA_Skill_DashSlash` plays MaskMan target sword dash montage when present, otherwise UEFN source fallback, applies one forward box hit on `AttackHit`, and ends/clears `Fixed` on `ActionEnd` or montage completion.
- 2026-05-29 MM debug source logging: user will verify via Live Coding; expected result is cyan/green `UEFNSource` log showing current DB, applied DB, validity, and selected animation.
- 2026-05-29 idle-to-short-walk fix in `UGP_CharacterAnimInstance`: user will verify via Live Coding; expected result is no idle-pose foot slide before start/loop motion appears on short taps.
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
