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
# Current Project State

Last synced: 2026-06-19T08:31:17+09:00

## Current Status

- FurnaceWalker enemy foundation created: `/Game/Characters/EnemyCharacter/Monsters/FurnaceWalker/BP_FurnaceWalker` inherits `BP_BasicEnemy_Melee`, keeps common AI/GAS melee setup, and now uses `/Game/Meshes/Monsters/FurnaceWalker/furnacewalker`. `RTG_FurnaceWalker` maps UEFN mannequin source to the FurnaceWalker IK rig. User added retargeted Idle/Jog, six attack, hit-react, and death sequences under `/Game/Characters/EnemyCharacter/Monsters/FurnaceWalker/Animations`. `/Game/Characters/EnemyCharacter/Monsters/FurnaceWalker/ABP_FurnaceWalker` is assigned to the child mesh and compiled: EventGraph calculates owner velocity magnitude into `Speed`; AnimGraph chooses Idle/Jog through a full-body `DefaultSlot`. `/Game/Characters/EnemyCharacter/Monsters/FurnaceWalker/AM_FW_Attack` remains `PDA_FW_AnimationSet.PrimaryAttackMontage` fallback, while six attack montages (`AM_FW_Zombie_Smashing_R/L`, `AM_FW_Zombie_Punching_R/L`, `AM_FW_Mutant_Punch_R/L`) are registered in `LightAttackMontages`. Each source attack sequence has `UGP_AnimNotify_SendGameplayEvent` for `GPTags.Event.Enemy.AttackHit` at common 45% timing and `GPTags.Event.Enemy.ActionEnd` near montage end. `GP_EnemyAttack` now chooses opposite-side L/R random first, avoids immediate repeats as fallback, listens for Enemy `ActionEnd`, and still ends on montage completion if the notify is missing. Pending: rebuild from correct engine/UBT path, then PIE-check locomotion, random attack selection, hit timing, ActionEnd release, and capsule/mesh offset.
- DragonSkull Control Rig setup is now in place for `/Game/Meshes/Monsters/DragonSkull/CR_DeagonBone_SimpleJaw`; `CR_DeagonBone` is not currently present after user refresh/reimport. Controls are `global_ctrl > root_ctrl > body_offset_ctrl > head_ctrl`, with `jaw_upper_ctrl` and `jaw_lower_ctrl` under `head_ctrl`. After asset normalization, stale offsets were rebuilt by placing controls from current skeleton bone initial transforms first, then parenting with maintain-global. Forward Solve drives translation/rotation only, not scale. Verified control positions match `root`, `Head`, `jaw_upper`, and `jaw_lower`, all controls scale `1`.
- `AGP_LevelBuildAnimator` now exists as a native runtime arena-construction prototype. It collects actors tagged `PLAZA_DE_TOROS`, stores their final transforms, generates underground/spiral/twisted start transforms, then builds them back with smooth per-piece interpolation. Ordering supports height-then-distance, height-then-random, distance-then-height, and random. Blueprint events expose build start/piece start/piece finish/finish hooks for Mirror Dimension VFX/audio polish. UHT passed; full compile is still blocked while Live Coding is active.
- Sans boss summon defaults to `/Game/Characters/EnemyCharacter/Basic/BP_BasicEnemy_Melee`; the removed legacy `/Game/Characters/EnemyCharacter/BP_Enemy_Melee` path is no longer referenced by `UGP_BossSummonAdds`.
- Sans Ground Hands is a native GAS pattern. `UGP_BossGroundHandsAttack` schedules three waves of three staggered strikes around the current player position; `AGP_BossGroundHandActor` owns the 0.75s red decal warning, primitive placeholder hand, floor trace, rise/retract motion, set-by-caller damage, and vertical launch. Generic boss selection/grants include `Attack_BossGroundHands`; Matador and Crystal Seraph continue through their specialized selector branches.
- Matador bull/decoy loop now has explicit bull states, proximity-based decoy redirect/return-hit radii, prototype player-overlap redirect via `TryRedirectTowardDecoy()`, decoy-hit success gated to `RedirectedByPlayer`, delayed groggy teleport to the decoy location, and a BP hook `BP_OnDecoyVanishRequested(VanishDelay)` for smoke vanish VFX. `GP_BullChargeActor` owns a native `BullActorVisual` child actor component using `/Game/Meshes/Bull/SK/Bull`, and `BP_Boss_Matador.BullChargeActorClass` points to native `/Script/Project_Eden.GP_BullChargeActor`. Redirect success is limited by charging state, one-time use, max distance, and facing angle; `UGP_MatadorBossStateComponent::TryRedirectActiveBullTowardDecoy(PlayerActor)` is the player parry/deflect call site. Decoy vanish spawns a temporary Niagara default `NS_Free_Magic_Hit2`, hides mesh, and disables collision. Matador state suppresses generic boss selector candidates (`Basic`, `Sweep`, `Area`, `Summon`), and `BTS_UpdateBossTactics` keeps Matador area disabled so bull/decoy remains primary pressure. Matador melee specials are native GAS abilities: `UGP_MatadorRapierThrustAbility` has 5s target tracking, rapier glow scalar, direction lock, 0.5s commit delay, 7500cm line-box thrust damage, and no C++ forward step so root-motion montages can own movement; `UGP_MatadorCapeGustAbility` has 1s prepare, direction lock, 30deg cone damage/knockback, optional slow GE, and low-health multi-burst. Bull and Cape/Rapier activation now mutually block through active bull checks and `MatadorMeleeActive`. `AGP_MatadorMageBossCharacter` grants them natively, and `BP_Boss_Matador` points to `/Game/GAS_Pattern/AbilitySystem/Abilities/EnemyAbilities/Matador/BP_GA_MatadorRapierThrust` and `BP_GA_MatadorCapeGust`. `BossAttackPatternSelector` scores Bull > Cape/Rapier, and `BTS_UpdateBossTactics` counts Matador melee range as an attack request to avoid chase/reposition stutter. External UBT remains blocked by active Live Coding, and MCP `LiveCoding.Compile` was issued after the bull proximity redirect + mutual exclusion patch.
- `BP_Boss_Matador` uses correct `BT_Boss_Matador` and `BB_Boss_Matador` overrides. The shared `EnemyAIController` boss runtime evaluation test cycle default is now disabled, preventing old Sans/common boss Basic/Sweep/Area/Summon sample cycling from driving Matador behavior unless explicitly re-enabled for testing.
- Matador bull pattern is now structured as a staged matador loop instead of a simple projectile: bull spawns away with a VFX hook/default Niagara, starts toward the decoy, first decoy contact triggers `BP_OnDecoyRedirectedBull` and redirects toward the player, player overlap/prototype redirect returns it to the decoy, and only that return hit records chain success. Non-final return calls `BP_OnDecoyBullReturned`; 3rd/groggy return calls decoy vanish/break and boss teleport flow.
- Basic enemy Blueprint inheritance path now exists. `AGP_MeleeEnemyCharacter`, `AGP_RangedEnemyCharacter`, and `AGP_FlyingEnemyCharacter` are Blueprintable C++ parents with archetype-specific sight/lose-sight/patrol/home ranges, built-in AI tuning, movement defaults, common `BT_EnemyCommon`/`BB_EnemyCommon`, and `/Game/Characters/MaskMan/SK_MaskMan` assigned as the prototype mesh.
- Enemy default GAS attacks are archetype-aware. `UGP_EnemyRangedAttack` replaces the shared overlap hit with `AGP_EnemyRangedProjectile`, aimed from its elevated spawn point to Blackboard `TargetActor`'s capsule center; the projectile supplies native collision/movement/prototype visuals and set-by-caller damage. `AGP_EnemyCharacter` grants the default attack ability, and shared `BTT_ExecuteEnemyAttack` resolves the archetype attack tag so ranged/flying parents keep the common BT.
- Basic enemy BP templates were created at `/Game/Characters/EnemyCharacter/Basic/BP_BasicEnemy_Melee`, `/Game/Characters/EnemyCharacter/Basic/BP_BasicEnemy_Ranged`, and `/Game/Characters/EnemyCharacter/Basic/BP_BasicEnemy_Flying`.
- Boss pattern execution is centralized in `BossAttackExecution`. `BTT_ExecuteBossAttack` and boss pawns accidentally routed through generic `BTT_ExecuteEnemyAttack` now use the same GAS pattern selector and activation/failure logging.
- `UBTT_ExecuteBossAttack` no longer reports success when GAS pattern activation fails. Granted-but-blocked/cooldowned boss pattern candidates are logged, the next scored candidate is attempted, and the BT task fails only when no candidate can activate.
- Crystal Seraph native boss prototype is implemented from `CrystalSeraphBoss_Plan.md`: `AGP_CrystalSeraphBossCharacter` uses requested `/Game/Characters/MaskMan/SK_MaskMan` as the prototype skeletal mesh, owns `UGP_CrystalSeraphStateComponent`, grants native Crystal Seraph pattern abilities, and exposes BlueprintCallable requests for prism, laser, shard, sanctuary, wing-core exposure, groggy, and recover.
- Crystal Seraph now enforces shared `BT_BossCommon`/`BB_BossCommon` in its native constructor and `OnConstruction`, retaining boss patrol, chase, and reposition flow. GAS activation atomically reserves a 2.5s shared attack cadence (groggy bypasses it), successful attacks immediately leave the Attack branch, and prism/laser readiness uses last-use cooldown timestamps instead of narrow world-time windows. Editor build and `ProjectEden.AI.Boss.PatternSelector.CrystalSeraph` passed; PIE patrol/chase validation remains.
- Crystal Seraph pattern actors exist in C++: `AGP_CrystalPrismActor`, `AGP_SeraphLaserActor`, `AGP_WingCoreHitActor`, `AGP_CrystalShardProjectile`, and `AGP_CrystalSanctuaryMarkerActor`. They use basic engine shapes as prototype visuals and expose BP events for polished VFX.
- Boss AI/GAS integration now has Crystal Seraph native tags, optional Blackboard keys, selector scoring, BT service/task mirroring, and damage-state multipliers: guarded `0.15`, wing-core exposed `0.5`, groggy `1.0`.
- Minimap terrain capture is resilient to missing level setup: `UGP_MinimapSubsystem` auto-spawns a transient `AGP_MinimapCaptureActor` when no placed capture actor exists, initializes the render target, and broadcasts it to HUD listeners.
- `AGP_MinimapCaptureActor` now renders scene primitives from a top-down capture, lazily reacquires the player pawn follow target, and updates capture around the player so terrain can appear in the HUD minimap.
- `UGP_PlayerHUDWidget` and `AGP_PlayerController` refresh the minimap background binding from the subsystem, covering late widget/player initialization and disabled widget tick cases.
- `WBP_CharacterStatsMenu` XP progress binding is repaired: `AGP_PlayerState::GetCurrentXP`, `GetCurrentLevel`, and `GetXPToNextLevel` are now cpp-defined BlueprintPure functions, UHT emits the exec functions, and `Project_EdenEditor Win64 Development` build succeeded.
- Enemy death now grants XP directly: `AGP_BaseCharacter` exposes a post-damage hook, and `AGP_EnemyCharacter` awards its editable `XPReward` once when health reaches zero by resolving the instigator's `AGP_PlayerState`.
- PlayerState XP debug can be enabled with `bDebugXPChanges`; `AddXP` multicasts a green on-screen/log message showing added XP, level transition, and current XP progress.
- PlayerState now has replicated XP/level basics: `CurrentXP`, `CurrentLevel`, `XPToNextLevel`, `LevelXPScale`, `AddXP`, server forwarding, and `OnLevelUp` Blueprint event for opening augment selection later.
- PlayerController now owns augment UI flow: `RequestOpenAugmentSelect` rolls candidates from the assigned pool, `OpenAugmentSelectWidget` displays them with Game+UI input mode, and `CloseAugmentSelectWidget` restores Game Only input. `UGP_AugmentSelectWidget` calls controller close after a selection.
- Augment selection now prevents duplicate picks: `UGP_SkillAugmentPoolData` can exclude already selected augments when rolling candidates, and `AGP_PlayerState::AddSkillAugment` ignores duplicate DA requests.
- Skill augment modifiers now include cooldown, projectile count, and element requirements. Empty `TargetSkillTags` applies to all skills; `RequiredElementTag` requires current tech element match. `CooldownMultiplier` is wired through `UGP_SkillBase::ApplyCooldown` and DashSlash fallback cooldown. `ProjectileCountBonus` is wired for SplitShot, NetTestProjectile, and ThrownBurst.
- Skill augment visual overrides are now wired: the latest applicable selected augment's `ActiveVFXOverride` and `ImpactVisualActorOverride` take priority over SkillData element/default visuals.
- `DamageMultiplier` now scales the complete skill damage formula, including attribute coefficient damage, through `Damage.Multiplier`; it no longer scales only base damage fields.
- PlayerController augment candidate generation now filters `RequiredElementTag` against the current tech element. Element-specific augments no longer appear before that element is selected.
- Selectable skill structure is now in C++: `UGP_TargetedSkillBase` supports `Instant`, `Projectile`, `Ray`, and `TargetActor` selection modes, preview actor/debug target updates, primary/secondary confirm, cancel, server-forwarded selection events, and aim-assist target actor selection near the aim line with range/LoS/filter checks. `UGP_Skill_NetTestProjectile` now uses projectile selection before commit/spawn; Dash/DashSlash cancel `GPTags.Ability.Skill.Selection`. `UGP_Skill_LifeDrainTarget` is a channeled target-select drain sample that continues while target distance <= selection range * 5 and LoS remains clear. Damage uses the configured/fallback damage GE; healing uses `/Game/GAS_Pattern/AbilitySystem/GameplayEffects/Healing/GE_Heal_Generic` with SetByCaller DataName `GPTags.Healing.Data.Base`.
- memoc commands now use project-local `.memoc/runtime` first. This avoids Codex sandbox timeouts when `.memoc/bin/memoc.cmd summary/search/doctor` tries to execute the global AppData runtime outside the workspace.
- ActionEnd lower-body handoff signal exists in C++ only for now. `Dash` and `DashSlash` request `SetActionLowerBodyMotionMatchBlendEnabled(true)` at ActionEnd/fallback ActionEnd, and `UGP_CharacterAnimInstance` exposes `ActionLowerBodyMotionMatchBlendAlpha` with interp speeds. The attempted AnimBP `LayeredBlendPerBone` graph insertion was reverted because it broke `DefaultSlot.Source` and caused non-montage animation A-pose. `ABP_UEFNSource_Player` and `ABP_MaskMan_Player` are restored to `pre-slot pose -> DefaultSlot -> Output`; a future lower-body handoff should use a cached-pose/safe single-consumer graph.
- Held movement now cancels action RM at ActionEnd without requiring a fresh key press. `AGP_PlayerCharacter` caches non-zero movement input even while `Fixed`, clears it on `Input_MoveCompleted`, and when `SetActionRootMotionInputCancelEnabled(true)` is called it broadcasts `OnActionRootMotionCancelInput` immediately if cached input is recent. `UGP_Skill_DashSlash` now registers/removes the same cancel delegate path as `UGP_Dash`.
- Log verification showed held cancel does fire for sprint-held fallback roll, but `ApplyCurrentActionInertia()` skipped because `ActionMotionCarryVelocity` had decayed to ~54 while entry speed was ~854. Held-input cancel now sets `bActionRootMotionCancelledByMovementInput` and seeds handoff velocity from max(carry, `ActionMotionEntryVelocity`, `CharacterMovement.MaxWalkSpeed`) in the held input direction, logging `[ActionRM][ApplyHeldInput]`.
- Direction-change slip after roll is being handled by feeding the held-input handoff velocity back into animation context. When held-input handoff seeds velocity, it also overwrites `HeldPostActionAnimVelocity`. `UGP_CharacterAnimInstance` now uses non-zero action motion velocity to override future `GeneratedTrajectory` sample positions for the UEFNSource AnimInstance, logging `[ActionRM][AnimTrajectory]`, so MM can see the intended direction immediately after ActionEnd instead of waiting for the normal trajectory component to recover.
- Trajectory override must not run during montage/root-motion playback. User observed good gameplay but odd vertical trajectory debug while rolling. The override condition was narrowed to `PlayerCharacter->IsUsingPostActionAnimVelocity()`, so roll playback keeps the normal `CharacterTrajectoryComponent` path and the synthetic future trajectory only exists for the short post-ActionEnd handoff window.
- Rare stop stutter after sprint-roll with released input was likely caused by unconditional post-action anim velocity hold from the last RM velocity. `ApplyCurrentActionInertia()` now creates `HeldPostActionAnimVelocity` only when cached move input is still recent or the action ended through movement-input cancel; released-input natural completion clears the hold.
- `ActionEnd` is control/input unlock, not montage end. For A-pose/default-pose bugs after RM actions, do not fix by changing RM/inertia end paths. Current trace patch adds `[ActionEndTrace]` logs; `UGP_Skill_DashSlash` treats `OnBlendOut` as log-only instead of completion. Logs showed fallback roll was not stopped by input in no-input tests; source fallback montage auto blend-out/default slot began before ability timer completion. After `Always Update Source Pose`, `AM_UEFN_Roll_RM` and `AM_UEFN_Sword_Dash_RM` BlendOut time is `0.25`.
- After enabling `Always Update Source Pose` on `ABP_UEFNSource_Player.DefaultSlot`, source fallback montage BlendOut was raised to `0.25`. `AGP_PlayerCharacter` now keeps a short post-action anim velocity (`PostActionAnimVelocityHoldTime=0.35`) from the last non-zero action motion velocity so UEFNSource MM can pick moving/stop poses after fast jump-roll instead of snapping to idle while capsule velocity/CMC velocity differ.
- Action root-motion movement is componentized as `authored/retargeted RM + shared carry`: target montages use their authored RM, source fallback montages use runtime-retarget consumed RM multiplied by `MovementSpeedScaleRatio`, and both add the same entry-velocity carry path. `BeginActionMotionTracking()` captures entry XY velocity then zeros CMC XY to avoid fallback double-consuming old movement velocity. Handoff applies remaining carry velocity only, not sampled RM+carry.
- UEFNSource fallback montage debug/anim speed reads `AGP_PlayerCharacter::GetCurrentActionMotionVelocity()` during fallback playback because manual `SafeMoveUpdatedComponent` root motion is not reflected in `CharacterMovement->Velocity`.
- Current roll RM source assets differ: `/Game/Characters/MaskMan/Animations/MaskMan_Roll_RM` root distance is ~593.9, while `/Game/Characters/UEFN_Mannequin/Animations/Montage/Roll/UEFN_Roll_RM` is ~465.5. With MaskMan `MovementSpeedScaleRatio=1.22`, fallback expected distance is ~568 before carry/collision.
- `AGP_PlayerCharacter.FallbackRootMotionDistanceCorrection` defaults to `1.18` to compensate observed runtime fallback loss: user measured target roll ~590-600 and fallback ~502 with only MovementSpeedScaleRatio; corrected expected fallback is ~592.
- DashSlash skill now exists as `UGP_Skill_DashSlash` plus `/Game/GAS_Pattern/AbilitySystem/Abilities/PlayerAbilities/Character1/GA_Skill_DashSlash` and `/Game/GAS_Pattern/AbilitySystem/SkillData/DA_Skill_DashSlash`. It plays `PDA_CharacterAnimationSet.SwordMontages.Dash_RM`, falls back to `SourceSwordMontages.Dash_RM`, and uses `GPTags.Cooldown.Skill.DashSlash` as a C++ cooldown fallback when the DA cooldown tag is blank.
- `UGP_CharacterAnimInstance` now treats ground acceleration input as movement intent before `Speed2D` crosses `IdleSpeedThreshold`, so short idle-to-walk taps can select start/move motion instead of dragging the idle pose while the capsule accelerates.
- Motion matching debug can now print from the `UEFNSource` anim instance with separate screen keys from the target mesh, including runtime DB, applied DB, result validity, and selected animation.
- `UGP_Primary::StartComboSequence` no longer rotates the player toward current movement input before starting a combo montage, so primary attacks use the character's current forward direction only.
- GAS damage/HUD triage: enemy and boss attacks now point/fallback to `/Game/GAS_Pattern/AbilitySystem/GameplayEffects/Damage/GE_PrimaryDamage`; player AttributeSet damage broadcasts use avatar fallback for PlayerState-owned ASCs.
- Player HUD GAS binding now resolves attribute widgets by name and binds boss health through `UGP_AttributeWidget`; `/Game/UI/HUD/WBP_PlayerHUDWidget.BossBar` is expected to be a `WBP_BossBar` child with Health/MaxHealth attributes.
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
# Current Project State

Last synced: 2026-05-23T00:00:00

## Current Status

- `UGP_SkillAugmentPoolData` is a new DataAsset for storing augment DA lists and `PickRandomAugments(Count)` returns valid non-duplicate random candidates for UI.
- `UGP_AugmentSelectWidget` is a new C++ parent for a 3-candidate augment selection widget; BP children should bind `Button_Augment0..2` and `TextBlock_Augment0..2`.
- Skill augment range scaling is wired for distance-style skills: `RangeMultiplier` affects LineShock length, ConeSlash hit range/visual offset, GroundBurst target distance, and MineBurst placement distance.
- Skill augment radius scaling is wired for burst-family skills: SkillBase resolves matching `RadiusMultiplier`, PulseBurst/GroundBurst scale their overlap radius directly, and MineBurst/ThrownBurst pass the multiplier into spawned mine/area projectile actors.
- Action root-motion movement is componentized as `authored/retargeted RM + shared carry`: target montages use their authored RM, source fallback montages use runtime-retarget consumed RM multiplied by `MovementSpeedScaleRatio`, and both add the same entry-velocity carry path. `BeginActionMotionTracking()` captures entry XY velocity then zeros CMC XY to avoid fallback double-consuming old movement velocity. Handoff applies remaining carry velocity only, not sampled RM+carry.
- UEFNSource fallback montage debug/anim speed reads `AGP_PlayerCharacter::GetCurrentActionMotionVelocity()` during fallback playback because manual `SafeMoveUpdatedComponent` root motion is not reflected in `CharacterMovement->Velocity`.
- Current roll RM source assets differ: `/Game/Characters/MaskMan/Animations/MaskMan_Roll_RM` root distance is ~593.9, while `/Game/Characters/UEFN_Mannequin/Animations/Montage/Roll/UEFN_Roll_RM` is ~465.5. With MaskMan `MovementSpeedScaleRatio=1.22`, fallback expected distance is ~568 before carry/collision.
- `AGP_PlayerCharacter.FallbackRootMotionDistanceCorrection` defaults to `1.18` to compensate observed runtime fallback loss: user measured target roll ~590-600 and fallback ~502 with only MovementSpeedScaleRatio; corrected expected fallback is ~592.
- DashSlash skill now exists as `UGP_Skill_DashSlash` plus `/Game/GAS_Pattern/AbilitySystem/Abilities/PlayerAbilities/Character1/GA_Skill_DashSlash` and `/Game/GAS_Pattern/AbilitySystem/SkillData/DA_Skill_DashSlash`. It plays `PDA_CharacterAnimationSet.SwordMontages.Dash_RM`, falls back to `SourceSwordMontages.Dash_RM`, and uses `GPTags.Cooldown.Skill.DashSlash` as a C++ cooldown fallback when the DA cooldown tag is blank.
- `UGP_CharacterAnimInstance` now treats ground acceleration input as movement intent before `Speed2D` crosses `IdleSpeedThreshold`, so short idle-to-walk taps can select start/move motion instead of dragging the idle pose while the capsule accelerates.
- Motion matching debug can now print from the `UEFNSource` anim instance with separate screen keys from the target mesh, including runtime DB, applied DB, result validity, and selected animation.
- `UGP_Primary::StartComboSequence` no longer rotates the player toward current movement input before starting a combo montage, so primary attacks use the character's current forward direction only.
- GAS damage/HUD triage: enemy and boss attacks now point/fallback to `/Game/GAS_Pattern/AbilitySystem/GameplayEffects/Damage/GE_PrimaryDamage`; player AttributeSet damage broadcasts use avatar fallback for PlayerState-owned ASCs.
- Player HUD GAS binding now resolves attribute widgets by name and binds boss health through `UGP_AttributeWidget`; `/Game/UI/HUD/WBP_PlayerHUDWidget.BossBar` is expected to be a `WBP_BossBar` child with Health/MaxHealth attributes.
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
# Current Project State

Last synced: 2026-05-23T00:00:00

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
- White Void 트랜지션 시스템을 대대적으로 리팩토링 및 안정화하였습니다. 멀티플레이어 환경에서의 simulated proxy 연출 동기화를 위해 `bIsInWhiteVoid`를 `ReplicatedUsing` 및 `OnRep_IsInWhiteVoid` 구조로 마이그레이션하였고, 중복 입력/타이머 처리로 인한 카메라 렉 오작동을 클래스 멤버 `RestoreLagTimerHandle` 및 Clear 보호 코드를 적용하여 안전하게 근절했습니다. 또한 모션 매칭 궤적 히스토리 리셋 시 구조체 변동에 견고하게 대처하도록 리플렉션 안전 조회를 구축했고, 애니메이션 인스턴스 틱 마비를 예방하기 위해 `NativeUpdateAnimation` 전체 얼리 리턴 구조를 제거하고 포즈 서치 연산만을 선별적으로 억제하는 방안으로 최적화하였습니다.

## Project Snapshot

<!-- memoc:snapshot:start -->
- Last synced: 2026-06-17T04:47:15
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
- Verify `UGP_AugmentSelectWidget` after editor rebuild: each augment DA's `AugmentType` should select the matching card background (`Dawn/Dusk/Midnight/Zenith` defaults).

## Completed Tasks

- Created `Diagonal_Path_Curvature_Analysis.md` containing diagnostic details on the forward-to-diagonal trajectory angularity.
- Resolved missing declaration `GetActiveMovementSpeedProfile` in `GP_PlayerCharacter.h`, fixing multiple C++ compilation errors and verified successful build.
- Resolved missing include `#include "GameplayTags/GP_Tags.h"` in `GP_BaseCharacter.cpp`, fixing compiler errors (C2653/C2065 for GPTags element variables) and verified clean compile of Project C++.
- Added `EGP_SkillAugmentType` (`BasicAttack`, `Skill`, `Ultimate`, `Passive`) to augment DAs and wired type-driven card backgrounds in `UGP_AugmentSelectWidget`.
- Renamed `WBP_TestAugmentSelect_1` card background images to `Image_CardBg0/1/2` for C++ optional binding.
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
