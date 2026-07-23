---
memoc: true
type: state
scope: project-memory
created: 2026-05-21T07:03:24
<<<<<<< HEAD
updated: 2026-07-23T19:03:00+09:00
=======
updated: 2026-07-24T07:01:06+09:00
>>>>>>> origin/main
status: active
tags:
  - memoc
  - memoc/state
---
# Decisions

Durable project decisions live here. Keep entries short, dated, and useful to future agents.

## Decision Log

<<<<<<< HEAD
### 2026-07-23
- Keep player attribute values live and reusable: Health/Mana/Stamina share one grayscale fill mask and one track brush, with color supplied by each ProgressBar tint. Preserve the existing `GP_AttributeWidget`, inner `HealthBar`, and parent HUD instance names. Apply the soft top-left backplate through the existing `TopLeftFrame` Border instead of rebuilding the UMG hierarchy; leave the optional accent unplaced unless later visual review asks for it.
- Keep village runtime PCG as Generate On Demand so the Director can apply instance-unique tags, graph parameters, and deterministic seeds before sequential generation. For immediate editor authoring feedback, use transient `ALevelInstance` previews owned by the Director, with PCG Preview editing mode and DuplicateTransient flags; do not change the production component to Generate At Runtime merely to see layouts.
- Treat the village Footprint component as a flat XY editor preview, not the runtime spawn origin or a selectable placement component. Keep it at slot-local Z=0 with minimal thickness and mark it visualization-only; the `AGP_VillageSlot` actor transform remains the authoritative Level Instance transform. Use pivot snapping (`Alt+End`) for authored ground placement.
- Keep the legacy `VillageLevelPreset` + footprint as the canonical primary preset so existing instance overrides remain valid. Store additional choices as `FGP_VillagePresetDefinition { PresetId, Level, Footprint, Weight }`; the first additional preset is compact `L_Village_01`. Ignore duplicate preset IDs or level paths deterministically.
- Assign presets before calculating conflicts, but retry up to 32 deterministic RunSeed/SlotId assignment salts and keep the successful result with the largest selected count. This preserves random visual variety without letting an unlucky large-preset assignment hide a valid mixed/small-preset layout.
- Preserve `AGP_VillageSlot`'s native root and render the assigned preset footprint through a child box so existing placed-actor transforms remain compatible.
- Treat village placement as a unit-scale, yaw-aware 2D footprint problem. Use exact XY OBB SAT for editor warnings and selection; terrain height does not make two villages spatially compatible. Selection must prioritize required groups and use deterministic feasibility/lookahead so an early bridge slot cannot block a valid later combination.

### 2026-07-22
- Preserve `PickCount` as the backward-compatible default for village group rules. Enable a separate `bUsePickCountRange` switch for inclusive `MinPickCount..MaxPickCount` selection so existing maps and their RNG sequence remain unchanged until explicitly migrated.
- Keep zero-village probability separate from count range: use `bRequired=false` plus `SpawnChance`. Range Min must remain at least one; required groups fail below Min while optional groups select the available clamped count.

### 2026-07-21
- Use an authored Level Instance preset rather than Runtime Data Layers for the first playable village activation. V1 loads only `/Game/WorldLayout/L_Village_00` at one selected slot, keeps it hidden until its PCG component is configured as On Demand/non-partitioned, then shows it and generates locally. The fixed source instance must remain absent from the main map.
- Defer multiple active villages until Road/District discovery is isolated per instance through unique tags and PCG graph-parameter overrides. Multiplayer client streaming and explicit cook registration are also follow-up work.

### 2026-07-20
- Build village randomization in two phases. V1 only propagates a per-run `RunSeed` and deterministically marks manually placed, stable-ID `AGP_VillageSlot` candidates selected/not selected by group. Do not activate candidate Runtime Data Layers or call city PCG yet: existing `BP_CityAnchor` content may generate on load, so real activation needs an explicit selected-layer contract, streaming-complete gate, city-PCG-complete gate, and vegetation ordering in V2. Keep Region Event zones independent until village placement is proven.
- Approve Candidate J geometry plus a production-safe variable blend half-width for V2.2. Keep hard IDs and packed visual IDs unchanged; vary only each region-pair's continuous visual influence from 18-32px around 24px with independent 80-240m low-frequency anchors. Each region samples its own nearest boundary-side pixel, while junctions, segment endpoints, and the perimeter return to the 24px baseline. This avoids cross-pair contamination and keeps the material/runtime contract unchanged.
- Historical stepping stone, superseded by Candidate J above: the first stronger V2.2 preset used 36-180m spacing, 2.5-15m amplitudes, and a 12m cap; it produced 10.575m actual maximum displacement and passed the then-current 51 checks.
- Do not judge Landscape V2.2 boundary visibility from an editor view until `RT_RegionState_15x1` contains at least two distinct states. A fresh editor map load can leave this transient render target black, making all four V2.2 slots resolve to the same surface even though the MI switch and Weight binding are correct. For non-persistent QA, use placed `BP_RegionStateManager.DebugRandomRegionStates` followed by `RefreshRegionTexture`; do not save the map solely for this validation state.

### 2026-07-19
- Disturb Landscape region borders offline in the V2.2 visual-weight generator, not with runtime shader noise. Use one independent non-uniform-anchor PCHIP per movable region-pair boundary: truncated-lognormal 36-140m gaps, Beta-distributed 1.75-9m magnitudes with local geometry caps/rare accents, 72% sign persistence, and dynamic 8-24m endpoint fades. Keep junction/perimeter cores fixed and limit each segment's signed mean to 10% of weighted RMS; this gives slower, larger asymmetric bends while the rasterized maximum region-area change stays below 0.5%. Do not normalize every segment to the same peak, force exact zero mean, alternate signs, or combine fixed coarse/detail bands, because those choices reveal a repeating S-wave cadence. Keep hard RegionID/PCG/StateRT and packed visual IDs exact.
- Make V2.2 the GameMap2 Landscape presentation path: keep the authoritative hard RegionID for gameplay/PCG, but visually blend up to four neighboring region states with fixed slots and normalized RGBA weights. Keep `UseRegionVisualBlendV22` default false in `M_StateMask`, enable it only on `MI_RegionLandscape_GameMap2`, and preserve V2.1/V2 as nested rollback paths.
- Store four visual IDs in two point-sampled G8 textures (`id0|id1<<4`, `id2|id3<<4`) instead of one G16 texture. G8 normalized samples decode all 256 bytes exactly through float16; G16 did not. Accept one extra sampler to avoid changing the heavy shared master's global precision mode, and keep the RGBA8 weight texture bilinear/uncompressed for validation.
- Resolve the remaining pair-switch and true-junction seams with Landscape-only V2.1: use an SDF-derived continuous Blend mask plus an auxiliary `R=pair-switch/G=junction` mask, and cover ambiguous cores with one replaceable neutral dirt/gravel surface. Keep its exact full-strength core near one source adjacency (~2 target texels), with smooth feathering outside it; do not increase RegionID resolution again.
- Gate both V2.1 texture selection and neutral-attribute blending behind the single default-false static bool `UseRegionTransitionV21`. Enable it only on `MI_RegionLandscape_GameMap2`, preserve V2/SeparateEdge as prerequisites, and leave PCG/other MIs pruned on their previous paths. Treat the current bright neutral dirt appearance as validation art that can be changed through unique texture parameters without rebuilding the region materials.
- Keep the V2 runtime contract owner-relative: decode canonical Pair `(A,B,OwnerIsB)`, reorder the states into Owner/Other, and blend `Owner -> Other` with unsigned Mix `0..0.5`. Canonical-B weight is validation-only. The state-texture A lookup must use canonical A directly, not the owner-valued `RegionID` output; otherwise one side becomes `B -> B` and produces a hard seam.
- Finish the Landscape-first rollout only on `MI_RegionLandscape_GameMap2`: save `UseRegionBlendV2=true`, PairV2/BlendV2, and the existing slope overlay; leave PCG and other MIs on legacy behavior. Treat the remaining sub-meter true triple-junction fallback and steep outer-wall projection stretch as separate follow-up issues rather than increasing ID texture resolution again.

### 2026-07-18
- Split the remaining Landscape visual defects into two independent fixes: use a GameMap2-only slope/cliff overlay for outer steep-wall XY projection stretch, and use canonical PairV2 `(A,B,OwnerIsB)` plus unsigned BlendV2 for long pair-switch stripes and junction discontinuities. Keep both features behind default-false static switches and leave PCG/other MIs unchanged. Two-ID data may retain a tiny hard fallback at true triple junctions.
- Supersede the topology-preserving 1024 seam repair as the final Landscape fix: reconstruct the current authoritative Region labels through per-label SDFs at 4096, derive the second-nearest label, and rebuild Pair and Edge together. Never regenerate from the stale seed list or nearest-upscale the 1024 image. Disable auto Virtual Texture on both outputs; keep Pair Nearest/NoMip and Edge Bilinear/mipped.
- Repair Landscape Edge seams without changing topology: preserve the filtered source mask, overlay a 2px fully saturated chamfer core with falloff to 8px from every R ownership transition, and keep Pair Nearest plus `BoundaryAlpha=Edge*0.5`.
- Roll out smooth RegionID boundaries to Landscape first: keep `M_StateMask.UseSeparateEdgeTexture` default false, enable it only on `MI_RegionLandscape_GameMap2`, and leave PCG on the legacy RegionID texture until a separate migration is requested.
- Runtime vegetation uses hierarchical grids with `GRID128` for the graph default and the current shared grass node at `GRID32`; `L_GameMap1`'s earlier 7,135-instance verification was performed before that authored grass change, at `GRID64`. Keep engine-default 2x generation radii, cleanup multiplier `1.5`, and bounded Surface Samplers.
- Keep the PCG Landscape Cache at `SerializeOnlyAtCook`: it works in PIE, serializes automatically while cooking, and avoids permanently embedding the cache in the editor map. `NeverSerialize` is invalid for Landscape-backed runtime PCG.
- Each map's placed `BP_VegetationSpawner` must have a Box that covers its Landscape. `L_LandscapeMap` uses `64000,64000,8000`, aligned horizontally to both 32m grass and 128m default grids while covering its 1.071km Landscape.
=======
### 2026-07-24
- Give Sans Ground Hands a dedicated deferred-decal material with a centered radial opacity mask, pure-red material parameter, and equal footprint axes. Do not reuse or parameter-hack the Sans Sweep fan material; the two warning silhouettes have separate production contracts.
- Keep the regular-enemy death grains at the designer-approved `User.SpriteSize=(10,10)`. Stage the motion as a readable fall followed by absorption: full gravity through `0.28s`, gravity fade to `0.60s`, attraction from `0.38s`, and a `0.80s` strength ramp.
- Use Niagara Point Attraction with falloff enabled and exponent `0` for constant acceleration, followed by Drag. This supersedes the original distance-scaled spring behavior, which synchronized distant grains and made them snap to the chest together.
- Revise the corpse-window tuning to `2.6x` playback, attraction strength `800`, drag `1.4`, kill radius `45cm`, and hard stop `1.90s`. Keep the existing two-second enemy despawn boundary and three-player target-latch policy.
- For regular-enemy death absorption, use the dead enemy as `User.SourceMesh` and a moving player chest `Position` as the destination instead of matching particles to a target skeletal mesh. Different enemy/player topology makes vertex correspondence brittle; one converging point reads more clearly and is cheaper.
- In the fixed three-player game, authority latches the valid killer once and otherwise chooses the nearest living connected player. Multicast that actor reliably; clients update only its live chest position and never rescore locally.
- Keep the absorption cosmetic within the default two-second corpse lifetime, stop the high-rate emission after a short window and at least three update frames, and use the latest timing values recorded above. Boss death presentation remains separate.
- Supersede the 2026-07-14 corruption/exploration decisions: the product has no world-corruption value or ambient/random Region Event path. Do not restore their timers, weighted selection, random placement, enemy scaling, outcome deltas, or presentation.
- Supersede the fixed-demo decisions dated 2026-07-21 and 2026-07-18 plus the remaining 2026-07-09 Region Event design: remove the seed-varied outer route, Red Rift/Defense/Shrine lifecycle, quorum/reward logic, gold objective marker, and automatic center boss/result path.
- Keep `RegionState` as an independent authored biome-ID/PCG contract, and keep general Zone/Portal/FinishRun code available. Neither system may implicitly restart the removed demo.
- Preserve the fixed three-player session and authored-anchor start expansion. Because protected `L_LandscapeMap` has no authored zones, its current post-lobby behavior is intentional free exploration until a separate gameplay-flow ticket is approved.
- Keep empty `GP_RegionEventDirector` and `GP_RegionEventData` serialization shells only while protected `L_LandscapeMap`/`DA_RegionEventData` retain those class references. They expose no event configuration or callable runtime API.

### 2026-07-21
- Lock enemy attack target identity at ability activation in the three-player game. Preparation and windup may sample that same actor's live position, but `AttackHit` locks direction and no target rescore may redirect the strike or projectile to a teammate.
- Keep regular-melee physical reach separate from runtime personality `PreferredRange`: forward-step melee starts at 350cm, in-place melee at 240cm, and cadence uses 275cm pursuit-entry / 225cm close-hold edges. Target changes reset attack-band hysteresis.
- One BT attack request activates exactly one exact-tag GAS spec, and one ability activation can apply its hit only once even when notify and fallback signals both fire.
- Treat enemy `ActionEnd` as gameplay-window closure, not montage completion. A shared enemy attack remains ability/BT-committed through the real montage end, recovery, and its exit bridge; only explicit incapacitation cancels it. Keep `AttackPrepare` and `ChaseResume` as optional per-enemy DataAsset seams with same-skeleton Idle fallbacks, and replicate the semantic phase rather than a runtime-created montage.
- Crystal Seraph's native shard projectile uses existing VFX mesh `SM_IceShard_03`, not the engine cone. The laser actor has no runtime VFX/mesh hardcoding: `BP_SeraphLaser` owns all four Niagara defaults and the production boss selects that child class. Use shared `#59ADFF` VFX tint for native pattern effects, reflected-beam lightning, and death shards/burst without modifying source Niagara or material assets.
- Keep the production Landscape architecture intact and layer the demo flow over it. The run uses one replicated authority director only on `L_LandscapeMap` when no legacy linear zones exist; it never calls the zero-zone `StartRun()` path.
- Preserve a fixed authored beat order while varying spatial execution by `RunSeed`: shared safe outer start, Red Rift, Structure Defense, Shrine, center rally, and Dark Armor Knight. This makes runs recognizable but not position-identical.
- Guided objectives use a two-of-three quorum clamped to currently possessed players. The Shrine rewards every connected party controller; the final rally normally requires two players but accepts one nearby player after a 45-second demo-safety watchdog.
- Reuse the existing minimap marker canvas and gold point texture for route guidance. Off-map objectives clamp in pixel space to the circular edge; red points remain enemy markers.

### 2026-07-18
- Treat Project Eden as a fixed three-player network game, not a minimum-three session: server admission is `3` players, `0` spectators, and `1` player per connection; only an exact three-player Ready party may travel.
- Keep ForceStart as an opt-in development escape hatch only. Shipping and remote clients always reject it; a non-Shipping local listen/standalone host must launch with `-AllowLobbyForceStart`.
- Once an enemy attack is committed, let it finish through ordinary target loss, disconnect, or leash reevaluation; only explicit incapacitating states such as death or groggy may interrupt it immediately. Never synthesize a fallback hit when a cancelled montage did not reach its hit event.
- While Matador's bull actor is live, the boss body stays stationary and the shared tactics service keeps the Attack branch committed. The bull actor owns an 18-second absolute lifecycle cap, while the BT task owns a 20-second external-action stuck cap and force-cleans the pattern on timeout/interruption.
- The July demo uses Dark Armor Knight as the representative boss and prioritizes basic melee/ranged enemies. Flying enemies and Motion/Pose Warping remain gated behind runtime proof; Motion Warping is only a post-P0 Dark Knight Charge pilot with a retained manual swept-movement fallback.
- Use the current `[EDEN-MAIN]` Codex thread as the single implementation and integration authority. Permanent DESIGN, CLIENT, WORLD, and QA threads provide read-only analysis and cross-review; the main thread relays messages because separate threads do not automatically share context.
- Split implementation into the smallest independently reviewable functional units. One unit normally becomes one `type(scope): short summary` commit with its directly related tests; unrelated changes stay in separate commits.
- Keep AI/GAS/network architecture and bulk-log specialists temporary rather than permanent, selecting a model and reasoning level from the bounded ticket risk.
- Build the graduation golden path around Lobby ready travel into `L_LandscapeMap`, then one discoverable corruption objective, cleanse feedback, augment, boss, and result. Expand breadth only after this 15–20 minute path is reliable.
- Treat dynamically named ServerTravel destinations as explicit Cook inputs. Editor package loading is not release proof; keep release status conditional until Cooked server/client travel passes.
>>>>>>> origin/main

### 2026-07-09
- Region events are a separate run-layer system, not a PCG graph mutation. `AGP_GameMode` only asks the placed/optional `AGP_RegionEventDirector` to roll events at zone boundaries; selected `AGP_RegionEventActor` instances own replicated presentation, optional enemy waves, and temporary/final region-state writes through `AGP_GameState`.
- Event-spawned enemies count toward the active zone clear budget. This keeps presentation events from being ignorable combat noise and lets designers use a region event as a real side encounter during a city clear.
- Concrete region-event examples are native actor classes selected by `UGP_RegionEventData.EventActorClass`. This keeps authored event pools in DataAssets while avoiding mandatory BP subclasses for common presentation-slice events.
- Crystal corruption nodes are non-ASC objective actors. Shared player combat overlap handles them explicitly before normal GAS application, so existing player attacks can destroy crystals without turning objectives into enemy characters.

### 2026-06-28
- Regular enemy cadence belongs to the enemy pawn and shared tactics service, not a fixed BT Wait asset. Roll only after GAS accepts an attack, close `bCanAttack` until the per-archetype timer expires, and exempt bosses so their pattern cadence remains authoritative.
- Footstep hearing is server-authoritative. Player movement reports hearing stimuli, while `AEnemyAIController` merges hearing and sight into the existing target-selection/BT pipeline instead of introducing a separate sound-only chase state.

### 2026-05-20
- Use `UEFNSourceMesh` as the runtime animation-driving source and let `CharacterMesh0`/MaskMan retarget from it.
- Prefer existing UEFN mannequin pose-search assets before building new databases.
- Keep chooser-driven database selection as the long-term direction, but keep explicit graph DB/state variables in `UGP_CharacterAnimInstance` because the current ABP graph uses fixed DB variables, not `RuntimePoseSearchDatabase` alone.
- Use the Blueprint `CharacterTrajectoryComponent` path for motion-matching trajectory when available.

### 2026-05-23
- Use `GP_CharacterAnimInstance` as the chooser context type instead of a specific ABP class so source/target anim instances do not type-mismatch.
- Treat MaskMan default movement speed around `500` as run-family motion and sprint around `700` as sprint-family motion.
- Use a dedicated `SprintSpeedThreshold` instead of broad run threshold reuse.

### 2026-06-04
- Add crouch locomotion by reusing existing `Stance` and crouch PSDs in `CHT_MM_MaskMan_Root_OriginalStyle`.
- Missing sparse crouch idle/TIP assets may fall back to Dense or Extreme Sparse PSDs.
- Hold crouch uses `IA_Crouch` on `C`, controller `Crouch()`/`UnCrouch()`, and character `bCanCrouch`.
- For crouch/uncrouch, do not patch visible dipping by stacking mesh component Z corrections. Leave mesh components alone first, then identify whether movement comes from capsule/root, attachment hierarchy, Retarget Pose From Mesh, or animation/root offsets.
- Prefer `Capsule -> UEFNSourceMesh` and `Capsule -> CharacterMesh0` sibling hierarchy over `Capsule -> UEFNSourceMesh -> CharacterMesh0`; preserve retargeting by connecting `ABP_MaskMan_Player` Retarget Pose From Mesh `SourceMeshComponent` to `RetargetSourceMesh`, filling it from C++, and ticking `UEFNSourceMesh` before `CharacterMesh0`.

### 2026-06-18
- Supersede the dedicated Crystal Seraph attack/idle tree: enforce `BT_BossCommon`/`BB_BossCommon` so patrol, chase, and reposition behavior stays shared while GAS scoring specializes attacks.
- Gate Crystal Seraph damage/teleport patterns through one boss-owned minimum cadence, but let the groggy state reaction bypass it.

### 2026-06-19
- Keep Sans Ground Hands in the shared boss GAS selector: the ability owns wave scheduling and cooldown, while replicated strike actors own telegraph, motion, damage, and launch presentation.

### 2026-06-20
- Enemy death follows the GAS boundary: `UGP_AttributeSet` detects terminal Health, `UGP_EnemyDeathAbility` orchestrates the server transition, and `AGP_EnemyCharacter` owns AI shutdown, collision/movement disablement, replication, and despawn.
- Keep death presentation separate from death rules. `BP_OnDeathStarted` and `OnEnemyDeathStarted` are optional hooks; no Blueprint setup is required for death correctness.
- Regular enemies own one native screen-space health bar through `AGP_EnemyCharacter`; `UGP_WidgetComponent` binds the shared Health/MaxHealth pair. Bosses remain on the dedicated HUD boss bar to avoid duplicate UI.

### 2026-06-21
- Keep Dark Armor Knight on shared Boss_Common navigation/targeting while its state component owns guard/parry/groggy truth, AI only scores ability tags, and GAS abilities/pattern actors own damage and collision. Use Blueprintable Engine-shape actors as replaceable art seams.
- Superseded by `93ac0b15`: crossing `ReturnHomeDistance` starts anchor return even with an active target. A visible player can re-engage after the enemy returns inside 75% of the outer distance, preventing boundary oscillation.
- Minimap FollowTarget alone owns periodic capture; FullMap capture is event-driven. Use orthographic FinalColorLDR for UMG-safe opaque alpha, then exclude lighting, shadows, translucent/particle VFX, decals, and debug overlays for 2D readability.
- Never swap or rebind the minimap RenderTarget sampled by UMG during periodic capture. Keep one stable display target, capture into an isolated back buffer, then GPU-copy completed pixels to the display target; hide the followed player because the HUD arrow already represents it.
- Advance the minimap transfer through capture-fence and display-copy-fence stages. Each RHI fence must have issued its write, drained pending writes, and passed `Poll()`; while incomplete, preserve the last displayed pixels and do not reuse the back buffer.
- Crystal Seraph reflection is the three-stage break mechanic. The third reflection enters groggy and falls; only the first player hit after landing starts the recovery delay, while Boss_Common remains paused.
- Keep Boss_Common assets shared, but enforce air/leash safety natively: flying vector MoveTo bypasses ground pathfinding while preserving altitude, Patrol yields when ReturnHome is active, and Crystal Seraph tactical teleports stay inside the anchor margin.

### 2026-06-23
- Ground Hands floor placement traces only WorldStatic geometry, never pawns. Keep the skeletal hand hidden for the entire decal warning and reveal it when rise begins; the native box remains the sole hit authority.
- Production boss danger ranges use floor decals rather than `DrawDebug` geometry. Sans Sweep uses one parameterized fan decal whose radius and angle mirror the real forward-arc hit calculation.
- Boss telegraph selection is two-stage: `Telegraph VFX On/Off` is only the master switch, and each non-Sans boss owns a BP-editable `Telegraph VFX Patterns` tag->bool map. Only checked pattern tags multicast the configured Niagara and wait its lead time; groggy/teleport state utilities bypass it. Reuse Dark Knight's existing BP component instead of adding a duplicate.
- Supersede periodic FollowTarget minimap capture with one post-PCG full-map capture. Keep the resulting RenderTarget fixed, disable SceneCapture after its fenced GPU copy, and move pan/zoom plus player/enemy marker presentation into C++/UMG.
- Boss target indication belongs to the boss pawn, not the BT task: `AEnemyAIController` only detects `TargetActor` acquisition/swap, then `AGP_EnemyCharacter` gates boss-only playback through `UGP_BossTargetMarkerVFXComponent`.

### 2026-06-22
- Resolve player SkillData and actor-owned boss Niagara through the same cue/element specificity function. Pattern actors own persistent effect lifetime; authoritative gameplay events multicast cosmetic one-shots.

### 2026-06-23
- Reinterpret the RegionState system as biome-type selection, not gameplay life/death/corruption status. Values should represent biome categories; GameMode terms like `AliveRegionState` / `DeadRegionState` are legacy naming to rename or replace when implementation resumes.
- Minimap correctness should not depend on level-authored PCG completion wiring. Keep a startup fallback full-map capture, allow later PCG-ready notifications to restart it, and make the HUD map image resolver tolerate production widget renames such as `MiniMapImage`.
- The minimap map texture should be clipped by the UI material, not only hidden under ring art. Player cursor heading should default to controller/view yaw so camera-facing a target matches the visible minimap direction.

<<<<<<< HEAD
### 2026-07-21

- Isolate streamed village PCG with per-slot Actor Tags plus component-local `UPCGGraphInstance` `RoadTag`/`DistrictTag` overrides. Remove template tags after retagging so legacy/global searches cannot mix simultaneous villages; do not mutate the shared base graph.
- Keep the automatically managed `PCGWorldActor` in `L_Village_00`; the Director only configures the explicitly identified `PCG_CityGen_FromAnchor` component. Remove fixed village Level Instances from the main map so the Director is the sole runtime owner.
- Treat multiple villages as an atomic presentation batch: all loaded/configured before visibility, all shown before generation, completion/cancellation delegates before success, and timeout/failure rollback. Multiplayer replication is a separate future feature.
- Run streamed city PCG sequentially after the all-shown barrier. Complex hierarchical graphs and engine PCG services are shared at world scope; one in-flight city graph removes cross-instance concurrency as a failure variable and spreads the generation spike. Preserve slot-unique tags and log PCG-managed output counts/bounds for runtime proof.

### 2026-07-23

- Keep the player top-left HUD focused on health, mana, and stamina. Remove the vignette, crest label, location label, and status hint; `LocationTextBlock` is optional in native code, so its absence is supported.
- Keep the player skill wheel at exactly eight clockwise slots using `DA_SkillPool_Player` order. With the current seven skills, the last slot is visible but inactive for future content.
- Use select-first equipment flow: choose a wheel skill, review it in the center, then click or press Q/E. The same skill cannot occupy both slots; selecting a skill from the opposite slot swaps the displaced skills, or moves it when the target is empty.
- Keep K as a toggle. While the menu is open, block only the local player's movement, look, and skill input through UI-only focus; never pause the multiplayer world.
- Preserve the legacy Widget Blueprints and construct the approved radial presentation as a defensive runtime fallback until an authored wheel hierarchy replaces it.
=======
### 2026-07-14
- Open-world exploration events are separate from the legacy linear-zone enemy budget. The event actor owns its spawned combatants and retires survivors through `RequestDeath`; zone/event tracking listens to the terminal `OnEnemyDeathStarted` delegate so scripted and combat deaths share one accounting path without granting cleanup XP.
- Preserve authored region seed biome values. Corruption affects event eligibility, probability, enemy GAS scaling, and outcome deltas; it does not flatten seed `State` values or write generic active/completed biome states.
- Production exploration pacing belongs to the placed `L_LandscapeMap` director: delayed/dwell-based evaluation, one active objective, global and per-region cooldowns, party-wide safe spawn distance, and non-deterministic event choice. Temporary PIE acceleration must remain unsaved and is guarded by map automation.
>>>>>>> origin/main
