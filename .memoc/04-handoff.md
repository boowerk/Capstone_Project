---
memoc: true
type: state
scope: project-memory
created: 2026-05-21T07:03:24
updated: 2026-07-24T06:02:46+09:00
status: active
tags:
  - memoc
  - memoc/state
---
# Agent Handoff

Last synced: 2026-07-24T06:02:46+09:00

## Regular Enemy Death Absorption Handoff

- Commits `34c84570` and `3633f215` add `/Game/Niagara/Dissolve_SK/NS_EnemyDeath_Absorb` plus `UGP_EnemyDeathAbsorptionComponent`; `9796240f` and `c971a3c2` add the falling phase and slower staggered absorption. Bosses remain on `UGP_BossDeathPresentationComponent`.
- Niagara uses `User.SourceMesh` for the actual corpse and keeps `User.SpriteSize=(10,10)`. Gravity runs before Point Attraction and Drag; falloff exponent `0` makes attraction constant instead of distance-scaled, so grains fall first and arrive at different times. Runtime follows the latched player's `spine_03`, updates fixed bounds over the source-target corridor, hides the source only after successful activation, and finishes before the default two-second despawn.
- Current timing/force defaults: full gravity through `0.28s`, fade to zero by `0.60s`, attraction delay `0.38s`, ramp `0.80s`, playback `2.6x`, strength `800`, gravity Z `-160`, drag `1.4`, kill radius `45cm`, and hard stop `1.90s`.
- Three-player policy is authority-only killer selection with nearest living connected-player fallback, then reliable multicast of that one actor. Dedicated servers skip local VFX; no client performs its own target selection.
- Verified after the falling-phase tuning: `Project_EdenEditor Win64 Development` build passed; `ProjectEden.VFX.EnemyDeathAbsorption.Policy`, `.ProductionAssetContract`, and `ProjectEden.Combat.EnemyDeath.Lifecycle` passed. Compiled module order is `Gravity -> Point Attraction -> Drag -> Solve`; relevant Niagara warnings/errors and NaN scans were zero. No live server/PIE run by request.
- Manual gate: kill several FurnaceWalker/Cyclops enemies at short and long range while the killer moves. If still too fast, try strength `600`; if distant grains fail to arrive, lower drag toward `1.1` before raising strength. Keep `DeathDespawnDelay` at least two seconds; lowering it can cut the attached effect short.

## Enemy Live-Target Combat Handoff

- Commits `bea85a35`, `a04c9d5e`, `e8dec768`, and `788facec` fix the observed Furnace 7m attack/cooldown idle, stale windup aim, duplicate hit/spec activation, unsafe forward-step lifetime, Cyclops no-step range, and three-player target-switch latch leak.
- Runtime contract: the attack locks one actor identity at activation, turns toward only that actor's live position through preparation/windup, locks at `AttackHit`, completes its montage, then chases during cadence outside the 275cm entry edge or smoothly faces inside the 225cm hold edge.
- Verified: `Project_EdenEditor Win64 Development` build succeeded and `ProjectEden.AI` passed 25/25. No content/map asset changed.
- Still manual: PIE-check Furnace lunge, Cyclops in-place hit, and ranged/flying shots against a strafing player. Live/server testing was intentionally skipped by request. Existing Cyclops AnimBP duplicate `DefaultSlot` warnings are unrelated and remain.

## Fixed Demo Removal Handoff

- Commits `c78e88a9`, `ba0f2bfa`, and `29baec37` remove the gold guided marker, DemoRun policy/director, run seed, peripheral start route, Red Rift/Defense/Shrine actors and DataAssets, event-specific augment RPC, automatic center Dark Armor Knight, and their dedicated tests.
- General three-player behavior remains: lobby/session cap, seamless travel, authored PlayerStart collection, collision-safe expansion to three stable slots, reconnect slot release, and the non-Shipping readiness probe.
- General Zone/Portal/Victory/Defeat/result/lobby-return code and RegionState/RegionSpatial/PCG biome state remain independent. `L_LandscapeMap` authors no enemy zones, so its current post-lobby state is three-player free exploration without an automatic objective, boss, or completion.
- Protected `L_LandscapeMap`, `TestMap`, `DA_RegionEventData`, and `L_MainMap` remain untouched. The placed `BP_EventDirector` and legacy data asset load through empty serialization-only native shells; those shells have no scheduling, spawn, reward, state, or completion API.
- Verified without a server/PIE session: full Editor build succeeded; `ProjectEden.Game.LandscapeMap.Integrity`, `ProjectEden.Game.Network.ThreePlayerRuntimeStarts`, `ProjectEden.UI.Minimap.CaptureStability`, and `ProjectEden.AI.Enemy.ProductionAnimationContract` passed 4/4 under `NullRHI`.

## Three-Player Network/Lobby Handoff

- Commits `5e0a4fcb` through `4ca8c603` establish the fixed three-player contract: lobby smoke count, duplicate-travel guard, three collision/ground-safe runtime starts, server/client gameplay readiness probes, a `3/0/1` GameSession cap in both maps, exact-three Ready gating, and local-debug-host-only ForceStart.
- `729d334d` removes the unused `BP_ProjectileMaster_ScaffoldBackup` that caused ten Blueprint compiler errors. WindowsServer Cook/Stage/Pak/Archive then completed with ExitCode 0 and no Blueprint errors.
- Confirmed baseline before the user stopped live server testing: packaged dedicated server plus three clients produced Ready `3`, all-ready `1`, ServerTravel `1`, three stable starts, server gameplay-ready `1`, client gameplay-ready `3`, and failure matches `0`. Server state was exactly three controllers/Pawns/owned gameplay classes at 260 cm separation; every client reported HUD, ASC, input, and mappings `2/2`.
- Current verification: Editor/Development Server builds pass and the combined AI, Dark Knight, lobby, and network automation regression is 30/30. Per user request, do not launch more live server/multi-client tests unless explicitly asked; implement server-compatible functionality and use builds/local automation.
- Preserve existing user edits in `TestMap.umap`, `DA_RegionEventData.uasset`, and `L_MainMap.umap`; all commits above excluded them.

## Graduation Demo AI Transition Handoff

- Commits `80f64845` through `a0c674e1` stabilize attack range transitions, smooth facing, latch actions through GAS, preserve committed targets, restore boss recovery, and close Matador bull lifecycle gaps.
- Latest focused fixes are `42e8cec8` (committed attacks survive target-loss/leash root reevaluation; explicit interrupts and cancellation policy), `668f7f37` (bull destroy cleanup plus 18-second absolute cap), and `a0c674e1` (stationary live-bull tactics hold plus 20-second BT stuck cap).
- `1ce0f79d` separates the gameplay `ActionEnd` signal from real montage completion; BlendOut no longer ends the ability and cuts its tail. `9e7dfeaf` adds `Face -> AttackPrepare -> GAS -> Recovery -> ChaseResume` inside the existing attack task. Both regular-enemy bridges use the owning DataAsset's Idle as a temporary same-skeleton fallback (`0.30/0.20s`) and expose replaceable clips/timing/blends/slot. A replicated phase lets three-player clients create the cosmetic dynamic montage locally while the server remains authoritative over timing.
- Verified: `Project_EdenEditor Win64 Development` build and all 23 `ProjectEden.AI` automation tests. No live server or PIE session was run by request. Existing missing Fab/UEFN assets and the Cyclops duplicate `DefaultSlot` warnings are not resolved by these commits.
- P0 before July 22/27: record three `Move -> AttackPrepare -> Hit -> full montage tail -> ChaseResume -> Move` cycles for each included enemy. Confirm one hit, no `Interrupted` callback on normal completion, no movement before the tail finishes, and visible preparation on FurnaceWalker/Cyclops. The loaded Furnace BT currently has editor strings for turn tasks but no compiled runtime turn-task instances; the new in-task bridge still runs, but compile/save that BT later if its separate turn nodes are expected. Do not run additional live network sessions unless the user explicitly asks; the product contract remains three players.
- The fixed demo flow is removed. For isolated AI presentation checks, use a temporary authored encounter without reconnecting DemoRun/RegionEvent systems; exclude flying unless its chase/altitude/re-entry loop passes PIE.
- Remaining engineering risks are exact GAS ability-spec tracking when duplicate attack-tag grants exist and simulated-proxy montage/yaw/root-motion behavior. Do not start the Motion Warping pilot until every P0 gate above is green.
- Preserve existing user edits in `TestMap.umap`, `DA_RegionEventData.uasset`, and `L_MainMap.umap`.

## Graduation Slice Lobby Entry Handoff

- `faa6c536` changes the native and production Blueprint lobby destination to `MainMap/L_LandscapeMap` and adds `ProjectEden.Game.Lobby.LandscapeTravelConfiguration`.
- `166b5e93` keeps the existing dedicated-server Cook maps and adds `/Game/Maps/MainMap/L_LandscapeMap` through `COOK_MAPS`, preventing the dynamic ServerTravel destination from being omitted by discovery.
- Verified: `Project_EdenEditor Win64 Development` build, lobby travel configuration automation, Landscape integrity automation, exact Cook-map declarations, UAT `COOK_MAPS` consumption, and destination package existence.
- Cook/package and the three-client Lobby-to-Landscape baseline now pass. Further live server testing is intentionally omitted per user request; retain only manual presentation gates and local build/automation checks unless asked otherwise.
- Protected `TestMap.umap`, `DA_RegionEventData.uasset`, and `L_MainMap.umap` remain untouched by this cleanup.

## Corruption and Event Runtime Removal History

- `ebd6847d`/`08861a81` removed world corruption and ambient/random events; `c78e88a9`/`ba0f2bfa`/`29baec37` then removed the remaining fixed demo event path.
- `RegionState`, `DA_NatureCorruptedVegetation`, and the 15 region seeds remain independent biome content. Do not remove them as part of event cleanup.
- If the protected map and legacy data asset are intentionally migrated later, remove the placed `BP_EventDirector`, `BP_EventDirector.uasset`, `DA_RegionEventData.uasset`, and then the two serialization-only native shells in one reviewed content ticket.

## Basic Enemy Cadence and Hearing Handoff

- Commits `24374376`, `f76cb69b`, `cbc771a8`, and `a1bbdc40` add per-archetype randomized attack cadence, shared BT enforcement, enemy hearing perception, and server-authoritative player footsteps.
- Full editor build passed. `ProjectEden.AI.Enemy.AttackCadence` and `ProjectEden.AI.Perception.FootstepNoise` pass.
- PIE-check several mixed basic enemies acquiring one player at once: first and repeated attacks should stagger, all intervals should stay below 3s, and moving behind a sight blocker inside HearingRange should acquire/refresh pursuit. Tune native/BP `Attack Cadence Settings`, `Hearing Range`, or player `Footstep Noise Settings` only if encounter feel needs adjustment.

## Matador Boss Pattern/AnimBP Handoff

- Current Matador design direction: the main boss body should mostly stay still as a guarded/vulnerable anchor. The active decoy performs visible combat patterns. The main body is pulled/teleported in only for groggy so the player can punish it.
- Code already mostly matches this: `AGP_MatadorMageBossCharacter::Tick()` calls `StopMainBodyMovement()` and `UpdateDecoyFollow()` while not groggy; `UGP_MatadorMeleeAbilityBase::ResolvePatternActor()` returns the active decoy when present, so Rapier/Cape pattern presentation and hit origin should be decoy-centered; bull pattern always uses `EnsureMatadorDecoy()` and sends the bull toward/through the decoy flow.
- Groggy flow already supports the intended punish target: `bTeleportToDecoyOnGroggy=true`, `GroggyDecoyTeleportDelay=0.35`, and `TeleportToPendingGroggyDecoyLocation()` move the main boss to the decoy position after chain break.
- Important AnimBP rule: do not animate the main boss as the normal attacker for Rapier/Cape/Bull. Put Rapier aim/lock/thrust, Cape prepare/gust, and Bull lure/redirect presentation on the decoy mesh/AnimBP. Main boss AnimBP should focus on idle/hover/guarded, groggy start/loop/recover, hit react, death, and possibly a subtle remote command pose only if it does not imply the boss body is the active hit source.
- Recommended assets: `ABP_MatadorBoss` for main body idle/groggy/punish states; `ABP_MatadorDecoy` for Rapier/Cape/Bull pattern performance; optional `ABP_Bull` only for visual bull run/redirect/hit presentation.
- Minimal implementation order for main thread: 1) create/assign decoy AnimBP and montages for `RapierPattern` and `CapePattern`; 2) wire BP events from `UGP_MatadorMeleeAbilities` (`BP_OnRapierAimStarted`, `BP_OnRapierDirectionLocked`, `BP_OnRapierThrust`, `BP_OnCapePrepareStarted`, `BP_OnCapeDirectionLocked`, `BP_OnCapeGustBurst`) to decoy montage/VFX; 3) add main boss groggy start/loop/recover montage; 4) add bull/chain feedback on decoy and chain actor.
- 2026-06-28 implementation update: `AGP_MatadorBossDecoyActor` now has decoy-side Rapier/Cape Blueprint events (`BP_OnDecoyRapierAimStarted`, `BP_OnDecoyRapierDirectionLocked`, `BP_OnDecoyRapierThrust`, `BP_OnDecoyCapePrepareStarted`, `BP_OnDecoyCapeDirectionLocked`, `BP_OnDecoyCapeGustBurst`), a `GetDecoyMesh()` getter, and optional `DecoyAnimClass`. Existing Rapier/Cape abilities call these events after their original ability BP events, so the current BP ability graphs remain compatible.
- 2026-06-28 implementation update: Rapier/Cape ability activation now notifies `UGP_MatadorDecoyPressureComponent` as pattern-action locked until `EndAbility`, so out-of-range teleport requests can wait for the current GAS/timer action to finish. Decoy break also stops the pressure component before hiding/despawning.
- 2026-06-28 asset check: MCP confirmed `BP_MatadorDecoy.DecoyMesh` currently uses `AnimationSingleNode` with `Male_Standing_Pose1` and no AnimClass. No blank ABP was auto-assigned because that would risk replacing the current visible pose with an un-authored ref pose. Next editor step is to create/assign a real `ABP_MatadorDecoy` to `DecoyAnimClass`.
- 2026-06-29 correction: decoy pressure is not distance keeping. It should walk in until skill staging range, then only teleport near the same chased player after sustained escape. Reserved teleports keep `CurrentTarget` unless invalid, and candidate order is randomized around that target. Decoy visual `StaticMeshComponent`s are forced `NoCollision` so the player can approach the decoy.
- 2026-06-29 follow-up: legacy `AGP_MatadorMageBossCharacter::UpdateDecoyFollow()` was still enforcing `DecoyFollowDesiredDistance=650` every tick, which made the decoy move away despite pressure movement. It now returns early while `UGP_MatadorDecoyPressureComponent::IsPressureActive()` is true.
- 2026-06-29 follow-up: pressure should not body-hug because Matador attacks with skill patterns, not basic melee. Defaults/BP values were retuned to `MeleeEnterRange=700`, `MeleeExitRange=1600`, `TargetOutOfRangeTime=2.25`, teleport `700/850/1000`. This is a one-way approach stop range, not distance keeping: if the player gets closer, pressure does not move the decoy backward.
- 2026-06-29 mesh note: Matador decoy has inherited `CharacterMesh0` plus explicit `DecoyMesh` because it inherits `AGP_EnemyCharacter`. `DecoyMesh` is the intended visible/animated mesh; inherited `GetMesh()` is now force-hidden/no-collision/no-overlap in `ApplyDefaultVisualLayout()` to prevent double visuals or phantom collision.
- 2026-06-29 animation update: native `UGP_MatadorDecoyAnimInstance` now derives from `UGP_CharacterAnimInstance`, preserving existing locomotion variables while exposing `bIsWalkingPressure`, `PressureState`, `CurrentTarget`, teleport lock, and `StepThrustIndex` for `ABP_MatadorDecoy`. `ABP_MatadorDecoy` was reparented/saved to `GP_MatadorDecoyAnimInstance` by editor Python commandlet. Next editor step is to compile/open-check the graph and wire the walk/step-thrust presentation.
- Verification target: in PIE, Rapier/Cape should visibly originate from the decoy, not the boss body. The boss body should remain mostly stationary until chain count reaches target, then move to the decoy/groggy location and become the punish target.

## Player Network Movement Handoff

- Client/server movement jitter was likely caused by directional `MaxWalkSpeed` mismatch. `AGP_PlayerController::ResolveEffectiveMoveInput()` now uses replicated/local raw move input first so server and owning client choose the same forward/side/back/sprint speed; server acceleration remains only as a fallback before input RPC arrival.
- `Project_EdenEditor Win64 Development` build passed. PIE-check with a separate server/client session for remaining camera shake; if any remains, inspect root-motion fallback actions next.

## Dark Armor Knight Boss Handoff

- Commits `4abb1dc1`, `85c40d78`, `0d0da2aa`, and `1c837936` implement the plan as native GAS/state/AI layers and create `/Game/Characters/EnemyCharacter/Boss/BP_Boss_DarkArmorKnight/BP_DarkArmorKnight` with `SK_KnightBoss`.
- Commits `b439e0f5` and `dfb86b4b` add the requested Easy Impact Frames lightning to the replicated charge actor. It auto-plays at the boss transform during the existing 0.9s warning, then `StartCharge()` begins root-motion movement. Editor build and `ProjectEden.Combat.DarkArmorKnight.ChargeTelegraphVFX` pass; PIE-check effect scale and whether 0.9s matches the visual strike finish.
- Commits `b451bf12` and `3e5dbf35` correct telegraph selection to per-pattern opt-in. `Telegraph VFX On/Off` is only the master switch; edit each boss Class Defaults `Telegraph VFX Patterns` map to check the exact tags that should use the cue. Crystal Seraph and Matador still inherit `BossTelegraphVFXComponent`; Dark Knight still reuses `GP_BossTelegraphVFX`. Dark Knight charge skips its own coordinator cue/delay only if the Charge tag is checked. Build, configuration/exclusion automation, and legacy charge automation pass; PIE-check timing and placement on selected attacks.
- Full editor build plus `ProjectEden.AI.Boss.PatternSelector.DarkArmorKnight` and `ProjectEden.Combat.DarkArmorKnight.GuardLifecycle` pass. Commandlet still reports the unrelated corrupt `Content/Maps/DemoMap/TestMap.umap` and missing Fab fence meshes.
- Commit `b7eb11d6` repairs the production BP's serialized empty Dark Knight ability array. Server-authoritative grant now keeps configured exact-tag replacements and fills only missing Basic/Heavy/Charge/DarkWave/GroundCrack/Groggy native specs. The production BP grant/Basic activation contract, all 6 Dark Knight combat tests, and all 21 AI tests pass; manual montage contact and Charge travel/timing remain P0.
- Commit `0a22e69e` fixes the boss stopping outside melee reach. Basic/Heavy use exact `350/420cm` damage ranges, Dark Wave is capped to its authored `520cm` slash, and Charge/GroundCrack stay ranged. The service continues Chase when no ready pattern can reach; the selector and execution context use the same ranges; GAS rejects an out-of-range committed target before cadence reservation. Delayed impacts intentionally do not recheck distance, preserving the player's wind-up dodge window. Editor build, Dark Knight 6/6, and AI 21/21 pass; no live server test was run.
- Editor work: place `BP_DarkArmorKnight`; assign/tune its Anim Class and mesh/capsule transform for `SK_KnightBoss`. For final art, create BP children of DarkWave/GroundCrack/Charge actors, replace their primitive component meshes/materials/VFX, then assign those classes on the boss. Optional Dark Knight Blackboard mirror keys are not required for runtime truth.

## Minimap Handoff

- Latest fix makes the minimap visually circular and aligns the player cursor with camera-facing direction: `M_UI_Minimap_StaticMap` is now a translucent UI material with `CircleMaskRadius`, and `UGP_PlayerHUDWidget` defaults the arrow to controller/view yaw. Build and `ProjectEden.UI.Minimap.CaptureStability` pass. PIE-check the screenshots' cases; tune `MinimapCircleMaskRadius` or `MinimapPlayerArrowAngleOffset` on `WBP_PlayerHUDWidget` only if art alignment still needs minor adjustment.
- Latest fix schedules a fallback one-shot capture when `UGP_MinimapSubsystem` first registers/resolves a capture actor, so missing `PcgControllerActor`/PCG-ready calls no longer leave the HUD on a blank generated RenderTarget. Explicit PCG-ready notifications are no longer ignored while the fallback is pending. `UGP_PlayerHUDWidget` now defaults `/Game/UI/HUD/Minimap/Materials/M_UI_Minimap_StaticMap` and resolves the production `MiniMapImage` name instead of relying on the old `MinimapBackgroundImage`. Build and `ProjectEden.UI.Minimap.CaptureStability` pass; PIE-check MainMap for final PCG coverage/orientation.
- Commits `a88bfee6`, `b3652834`, and `e6806c69` replace runtime follow capture with one full-map capture after PCG becomes idle. The stable texture is panned/zoomed by `M_UI_Minimap_StaticMap`; player arrow and pooled red enemy markers are separate UMG widgets. Assign `/Game/UI/HUD/Minimap/Materials/M_UI_Minimap_StaticMap` to `WBP_PlayerHUDWidget.MinimapMapMaterial`, then PIE-check map orientation, coverage, zoom, and markers. Editor build and real D3D12 offscreen `ProjectEden.UI.Minimap.CaptureStability` passed with no material compile warning.
- Commits `42b72f57` and `19340f27` remove periodic RenderTarget swapping/rebinding. UMG keeps one stable display target while SceneCapture remains isolated on a back buffer; completed captures are GPU-copied only after the capture fence, and the pipeline remains blocked until the copy fence completes. Editor build plus NullRHI and D3D12 offscreen `ProjectEden.UI.Minimap.CaptureStability` passed. No editor setup is required; PIE-check repeated attack Niagara visually.
- Commits `23aedf2b` and `0bfa780f` replace render-command-only promotion with an `FRHIGPUFence` completion gate. The old front buffer remains bound until the capture fence write is issued, pending writes drain, and `Poll()` succeeds. Editor build, NullRHI automation, and real D3D12 offscreen `ProjectEden.UI.Minimap.CaptureStability` passed; PIE-check attack Niagara visually.
- Commits `d64bc60c` and `88d85765` prevent attack-time flicker with front/back render targets, RHI-fence promotion, and player exclusion. Build and `ProjectEden.UI.Minimap.CaptureStability` passed; PIE-check repeated attacks with production VFX. No editor setup is required.
- Commits `2641dbab`, `c46e718f`, and tests fix FullMap Z accumulation, preserve Follow mode, stop brush reflow, and use flat opaque FinalColorLDR. BaseColor was rejected because its alpha can make direct UMG display transparent. Build and `ProjectEden.UI.Minimap.CaptureStability` passed; PIE-check final contrast.

## Enemy Leash Handoff

- Commits `93ac0b15` and `177bc9ad` finalize anchor leash behavior: crossing the outer distance starts return, and a visible player re-engages inside the default 75% inner boundary. Editor build and `ProjectEden.AI.Enemy.LeashPolicy` passed; PIE-check actual BT movement at both boundaries.

## Enemy Health Bar Handoff

- Commits `6c668c6b` and `130afeb2` attach `WBP_EnemyHealthBar` to the shared enemy parent at Z=135, keep it visible at full health, bind GAS Health/MaxHealth, hide it for bosses/dead enemies, and add a passing defaults test. Editor build passed; PIE visual placement still needs checking on differently sized meshes.

## Enemy Death Handoff

- Commits `4783ba61` and `a0ae0cb4` implement shared HP-zero death for melee, ranged, flying, and boss children of `AGP_EnemyCharacter`. Editor build and `ProjectEden.Combat.EnemyDeath.Lifecycle` passed.
- PIE-check representative enemy/Boss assets for lethal damage and the 2-second default despawn. No editor assignment is required; later animation work should implement `BP_OnDeathStarted` and tune `DeathDespawnDelay`.

## Boss Target Marker VFX Handoff

- Latest fix makes boss target marker VFX death-safe: the boss-owned marker component tracks spawned player-attached Niagara components, clears them on death/EndPlay, and blocks delayed play RPCs after owner death. Editor build and `ProjectEden.Combat.Boss.TargetMarkerVFXConfiguration` pass.
- PIE-check a multiplayer boss fight where the boss selects a target, swaps targets, then dies; the torso marker should vanish on every client and not reappear after delayed network messages.

## FurnaceWalker Handoff

- `ABP_FurnaceWalker`: stationary uses full-body `DefaultSlot`; movement layers its attack pose from `spine_01` over locomotion. Existing Idle/Jog pin order is user-corrected; do not rewire it blindly. ABP changes remain uncompiled/unsaved by request.
- `UPDA_EnemyAnimationSet` is newly added but not built. `AGP_EnemyCharacter` exposes `EnemyAnimationSet` and applies its mesh/AnimBP; `UGP_EnemyAttack` reads its attacks, falling back to legacy `PDA_CharacterAnimationSet` for unmigrated bosses. After external build, create and assign `PDA_FW_EnemyAnimationSet`, then clear the legacy `AnimationSet` on `BP_FurnaceWalker`.
- FurnaceWalker target attacks: Mutant Punch L/R and Zombie Smashing L/R. Each selected montage has direct `AttackHit`/`ActionEnd` events; all use 0.25s Blend Out. Smashing events: 1.63s/3.45s.
- No MCP build: use the user's correct engine/UBT workflow, then PIE-check mesh/ABP assignment, random alternating attacks, event damage/release, and upper-body movement attacks.

## Sans Ground Hands Handoff

- Commit `e080f16c` updates `UGP_BossSummonAdds` to spawn `Basic/BP_BasicEnemy_Melee`; full editor build passed and startup logs showed no class-finder failure for the new path.
- Commits `f4e75d83` and `8a7fbc60` add the native strike actor plus `Attack_BossGroundHands` GAS ability/selector/default grant. Defaults are 3 hands per wave, 3 waves, 0.22s hand stagger, 1.55s wave interval, 0.75s red decal warning, and 8s ability cooldown.
- Commits `72ed5c6c`, `d7cc3dc4`, and `7a25612b` replace the primitive hand with `/Game/Meshes/PLAZA_DE_TOROS/ActorMesh/SK_RightHand`, remove legacy static-mesh placeholders, and preserve the independent box hit settings. The previously stale editor DLL was fully relinked; `ProjectEden.AI.Boss.GroundHands.UsesRightHandMesh` passes.
- Commits `8763987f`, `23bbe52d`, and `dfb4ea20` add presentation-only Scale/Offset/Rotation controls, create `/Game/Characters/EnemyCharacter/Boss/BP_Boss_Sans/BP_BossGroundHandActor` at scale 0.35, and route the GAS ability through it. The BP can be tuned in Class Defaults; collision remains native and unchanged. Editor build and both `ProjectEden.AI.Boss.GroundHands` tests pass.
- Commits `4ba556b5`, `9174e93a`, and `6f31e9a4` keep the center hand's floor trace from hitting the player capsule, preserve every warning decal, and hide each hand mesh until its rise begins. The scale test now permits later BP art tuning while requiring a positive reduced scale. Editor build and both GroundHands tests pass.
- Commits `797ba8c7`, `c3c0f458`, and `13707ad9` replace Sans Sweep debug lines with `/Game/Effects/M_BossSweepTelegraph_Decal`, a dynamic red fan decal matching the attack radius/angle. Editor build and `ProjectEden.AI.Boss.Sweep.UsesFloorDecal` pass; PIE-check forward orientation and projection across sloped ground.
- Full `Project_EdenEditor Win64 Development` build and `ProjectEden.AI.Boss.PatternSelector.SansGroundHands` passed. PIE still needs visual timing, decal projection, hit collision, damage, and vertical launch/fall verification.
- The broader `ScoreCases` test still has an unrelated existing Matador expectation mismatch (Cape expected, Rapier selected); the Sans-specific test is isolated and green.

## Basic Ranged Enemy Handoff

- Commits `573e1707`, `ab0171f4`, and `d526e4f9` add an overridable shared hit point and a native player-aimed projectile for `BP_BasicEnemy_Ranged` while retaining `BT_EnemyCommon` patrol/chase/attack flow. Aim is recalculated from the elevated spawn point to the player capsule center.
- Full `Project_EdenEditor Win64 Development` build passed. PIE-check the 850 cm attack band, trajectory, and damage.

## Crystal Seraph Boss Handoff

- Latest animation/VFX polish keeps `/Game/Characters/EnemyCharacter/Boss/BP_Boss_CrystalSeraph/Animation/ABP_CrystalSeraph`, `PDA_CrystalSeraphAnimationSet`, and multicast pattern montage playback, but Basic/Shard and Laser montages now compose Enter→Shoot→Hold→Exit so the attack pose remains visible briefly after firing. Pattern actors use duplicated `/Game/Characters/EnemyCharacter/Boss/BP_Boss_CrystalSeraph/VFX/NS_CrystalSeraph_*` Niagara assets plus `UGP_VisualCueComponent` tint `59ADFFFF`; original Free_Magic assets are not modified. Build, `ProjectEden.Combat.CrystalSeraph.AnimationSetup`, and `ProjectEden.Combat.CrystalSeraph.VisualCues` pass. PIE-check final animation timing and tint intensity.
- Latest restore fixes the missing Blueprint symptom: `BP_Crystal_Seraph.uasset` had been committed as an LFS object containing merge-conflict pointer text in `82b7d607` and then kept by merge `3f13056e`. It is now restored from the valid `59fa4cfb` version. Open the BP in editor and compile/save if prompted.
- Commits `c8aac963` and `3dd2fb6b` restore zero pitch/roll prism placement and reduce editable `PrismAuraScale` from 1.25 to 0.55. All changed translation units compile, but final DLL link and test execution are pending because the open Unreal Editor holds `UnrealEditor-Project_Eden.dll`; close the editor and rebuild.
- Commits `50569b35`, `5fab4c1f`, and `7ed563c2` share the player visual-cue resolver with actor-owned presentation and add default Niagara to prism, shard, laser, and sanctuary patterns. Full build and all `ProjectEden.Combat.CrystalSeraph` tests pass. No required editor assignment; PIE-check asset scale/orientation and override each actor's `VisualCueComponent.VisualCues` in BP if art tuning is needed.
- Commits `d79c7f35` and `b12a5dd5` enlarge the prototype prism and spawn three non-overlapping crystals on a 650cm target-centered ring. Editor build, PrismCluster, and existing GroggyLifecycle tests pass; PIE-check visual spacing and tune `PrismVisualScale` / `PrismRingRadius` only if needed.
- Commits `a70a62cd` and `416d16f5` fix the logged teleport-to-leash-to-Patrol loop without editing BT assets. Tactical teleports clamp inside the anchor margin, the Patrol EQS task yields to ReturnHome, and flying vector moves use direct altitude-preserving paths. Build and PatrolRecovery/GroggyLifecycle/selector tests pass; PIE-check that Patrol fallback logs no longer spam.
- Commits `7a94b1ac` and `8dfa6c98` route each reflected laser into one break stage; the third causes a gravity fall. Falling hits do not count, the first grounded player hit starts `GroggyDuration` (final phase: `FinalPhaseGroggyDuration`), and Boss_Common resumes on return to hover. Build plus lifecycle/selector tests pass; PIE-check presentation and tune only those durations if desired.
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

- After the demo deadline, remove the obsolete EarlyTransition notify state from the 22 UEFN Run/Slide animation sequences and resave them. Do not restore the deleted Sandbox ABP dependency stack just to silence these warnings.
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

- 2026-07-21 origin merge: `Project_EdenEditor Win64 Development` build passed; `ProjectEden.AI` 22/22, `ProjectEden.Combat.DarkArmorKnight` 6/6, `ProjectEden.Game.Network` 2/2, and `ProjectEden.Game.Lobby` 2/2 passed without live server execution.
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

- 2026-07-23 Prism shield-surface reflection and `PrismBodyCollision` passed full UBT. PIE-check body blocking, foot IK, and surface reflection location.
- 2026-07-23 Reflection now disables the incoming laser `DamageBox` and spawns its outbound segment visual-only. The source compiled successfully, but final DLL linking is pending because `UnrealEditor.exe` still holds `UnrealEditor-Project_Eden.dll`. PIE-check that shielded reflection deals no player damage while an unreflected beam still does.
- 2026-06-24 boss target marker VFX death cleanup is built and automated, but PIE still needs a multiplayer visual check for death, first acquisition, and target swaps.
- 2026-06-24 Crystal Seraph BP LFS restore is committed locally; editor visual check is still needed because commandlet startup hit unrelated project load issues (`EventMap2.umap`, missing Fab fence meshes).
- 2026-06-23 Sans Ground Hands decal/visibility fixes are built and automated coverage passes; PIE-check all three decals per wave and confirm each hand appears only as its rise begins.
- 2026-06-23 Sans Sweep fan decal is built and automated coverage passes; PIE-check that the fan points forward and visually matches the 165-degree damage arc on uneven terrain.
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
