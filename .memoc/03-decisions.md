---
memoc: true
type: state
scope: project-memory
created: 2026-05-21T07:03:24
updated: 2026-08-13T03:36:00+09:00
status: active
tags:
  - memoc
  - memoc/state
---
# Decisions

Durable project decisions live here. Keep entries short, dated, and useful to future agents.

## Decision Log

### 2026-08-13
- Publish Project Eden proof as privacy-safe PNG and short self-hosted GIF assets; do not create MOV or invent an external evidence-video URL.
- Treat the successful world-sync run as Dedicated Server evidence for Snapshot, local generation, first-attempt ACK approval, and per-client placement only. It does not prove ACK retry or player movement.
- Treat the PlayerState before/after composite as evidence of server-authority `AddXP(125)` and the `CurrentLevel` RepNotify augment-picker path on three remote clients. It does not prove numeric XP/level parity, selected-augment replication, equipment, or elimination state.

### 2026-08-12
- Describe village placement as a per-client latest-Revision gate, not an all-client ACK barrier. Placement also requires zone progression, a valid Pawn, and an Outer PlayerStart; a Listen Server's local host is exempt from the ACK check.
- Treat the public Project Eden team demo only as actual team gameplay/environment evidence and its final split view as a 2-player result. It does not prove 3 clients, Dedicated/Listen NetMode, Snapshot/ACK/retry/gate behavior, deterministic selection, or PlayerState RepNotify.
- Keep automation evidence distinct from multiplayer evidence. The privacy-safe Village Selection PNG is a local summary rendered from a fresh Unreal Automation exported JSON result, not a direct Automation UI screenshot.

### 2026-07-26
- Preserve explicit F1 attribute and F9 Encounter UMG debug tools, but keep transient engine screen messages and gameplay-world debug primitives out of normal play. Monster/skill `DrawDebug*` code may remain for development only behind a default-off non-Shipping opt-in; Shipping must reject it.
- Treat production decals, Niagara cues, preview actors, and attack telegraphs as gameplay presentation rather than debug output. Do not hide them through the debug gate.
- Debug-log flags may control diagnostics only. They must not gate motion correction, damage, targeting, replication, timers, or other gameplay behavior.

### 2026-07-27
- Treat every connected player as the same co-op party until an explicit team system exists. Block player-to-player combat effects before presentation/knockback and keep a second authoritative damage-execution guard; direct-spec abilities such as Life Drain must recheck the same policy.

### 2026-07-24
- Keep elimination/spectating/run-result input state alongside the initial Outer loading gate; neither flow may erase the other's declarations or state. Client village readiness must retry until a local controller exists instead of dropping its one-time ACK.
- Keep `T_GameMap1_RegionID` at GPU availability now that vegetation uses server-cookable graph/world data; CPU availability did not preserve pixels in WindowsServer cooks and only leaves unnecessary client CPU data.
- Produce final Windows releases with the one-shot build/cook script and require a clean Git tree unless dirty output is explicitly requested. The current installed engine supports only Development Game/Server targets, so label and deploy Development honestly; do not edit installed-platform metadata to fake Shipping support. A future Shipping release requires rebuilding the engine distribution with Shipping Game and Server targets. Deploy complete Client/Windows and Server/WindowsServer folders and never add `-AllowLobbyForceStart`.
- Own the initial Outer loading overlay in `UGP_GameInstance` so it survives seamless travel. Start from the lobby loading RPC or direct gameplay join, but never hide on map load, possession, or village visual-ready ACK; hide only after the server teleports successfully and the owning client observes its pawn at the assigned Outer location.
- Keep `PCG_Vegetation_Global` active on both clients and dedicated servers, but do not sample RegionID texture pixels in its server path. WindowsServer cooks strip texture platform pixels even when Availability is CPU, so vegetation inputs needed by the server must be server-cookable graph/world data rather than `UTexture` pixel data.
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

### 2026-07-14
- Open-world exploration events are separate from the legacy linear-zone enemy budget. The event actor owns its spawned combatants and retires survivors through `RequestDeath`; zone/event tracking listens to the terminal `OnEnemyDeathStarted` delegate so scripted and combat deaths share one accounting path without granting cleanup XP.
- Preserve authored region seed biome values. Corruption affects event eligibility, probability, enemy GAS scaling, and outcome deltas; it does not flatten seed `State` values or write generic active/completed biome states.
- Production exploration pacing belongs to the placed `L_LandscapeMap` director: delayed/dwell-based evaluation, one active objective, global and per-region cooldowns, party-wide safe spawn distance, and non-deterministic event choice. Temporary PIE acceleration must remain unsaved and is guarded by map automation.

### 2026-07-26
- Player recovery respects staged progression: an incomplete assigned Outer recovers at that village's existing `PlayerStart`; after that Outer is complete, recovery may join a living teammate.
- Keep the normal Windows-subsystem dedicated-server executable for deployment compatibility and build a `-Cmd.exe` sibling for local administration. Launcher BATs prefer the console sibling and use Unreal's native package/sandbox log path instead of assuming the editor `Saved/Logs` directory.
- Treat tagged enemy spawn points as trusted ground anchors: project them with a tight vertical extent, scatter only on the same reachable NavMesh island, and fail closed while authored anchors are unavailable. Never widen to arbitrary roof NavMesh as a fallback.
- Keep the boss-only broad vertical projection for Level Instance offsets, but accept it only when a tight ground anchor can path to the candidate. Revalidate the spawned capsule foot separately with a strict `50cm` upward allowance and retry unsafe or partial boss placement.
- Treat every valid configured enemy count as an encounter obligation. Safe-placement timeouts are diagnostic thresholds only; keep the count pending and retry rather than silently shrinking a marker, zone batch, staged portal, or active-Colosseum relocation.
- Boss-summoned adds preserve their existing pressure-only role: do not register them in zone completion counts. On summoner death, retire them through `RequestDeath` with the real death instigator so ordinary VFX and cleanup still run.
- Re-evaluate party-gated Center/Colosseum progression one tick after `Logout`, because Controller cleanup removes the departing PlayerState only after `GameMode::Logout` returns. When a run finishes, clear object-bound retry timers before creating the lobby-return timer.
- Use a dedicated emissive circular deferred-decal material for player ground targeting and Dark Knight ground warnings. Keep region landscape materials and lighting untouched; visibility-critical combat warnings must remain readable through their own Emissive output.
- Keep the shared boss telegraph default unchanged for other bosses. Dark Knight overrides it at construction and BeginPlay with the sprite-only `NS_Lightning_Owner_Cast`, and its native Charge fallback uses the same system so the old example Niagara mesh renderers cannot reappear.
- Preserve Dark Knight gameplay geometry while replacing presentation primitives: Charge keeps its 1600x360 warning as a projected decal, Ground Crack keeps its 240cm radius as a decal, and Dark Wave keeps its collision box while using a Niagara component.
- Validate spawned enemies against the collision-adjusted capsule foot captured after component initialization but before `BeginPlay`. This preserves strict grounded roof rejection while allowing flying bosses such as Crystal Seraph to move vertically during `BeginPlay`.
- Dark Knight attack montages remain server-authoritative and are mirrored through reliable multicasts; dedicated servers always evaluate the pose so charge root motion stays authoritative. Groggy interruption uses the same network presentation boundary.
- Restore Dark Wave as a projectile attack without retaining the replacement cone hit. Staged portals resolve destinations from grounded authored `EnemySpawnPoint` anchors rather than an unprojected zone-box center.
