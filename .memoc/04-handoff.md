---
memoc: true
type: state
scope: project-memory
created: 2026-05-21T07:03:24
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
updated: 2026-07-23T17:08:56+09:00
=======
updated: 2026-07-24T07:01:06+09:00
>>>>>>> origin/main
=======
updated: 2026-07-24T07:01:06+09:00
>>>>>>> origin/main
=======
updated: 2026-07-24T07:01:06+09:00
>>>>>>> origin/main
status: active
tags:
  - memoc
  - memoc/state
---
# Agent Handoff

## Runtime Debug Visibility Handoff (2026-07-26)

- Branch `fix/runtime-debug-visibility` keeps the F1 attribute widget and F9 Encounter panel unchanged while disabling transient engine screen messages and default gameplay-world debug primitives.
- `g.DrawSkillDebug` defaults to `0`, is non-Shipping development opt-in only, and is consulted by skill overlap helpers plus Bull/Chain/Matador/LifeDrain/Projectile paths. Village selection drawing is separately default-off and double-opted-in.
- Production decals, Niagara, preview actors, damage/overlap results, replication, and timers were not routed through the debug gate. Post-action motion trajectory correction was separated from `bEnableDebugLog`.
- Verification: `Project_EdenEditor Win64 Development` succeeded; full `ProjectEden` automation passed 70/70; direct C++ `AddOnScreenDebugMessage`/`PrintString` search is empty; `git diff --check` passed.
- Manual PIE gate: trigger representative player skills and Matador/Dark Knight attacks, confirm no debug lines/boxes/spheres or transient screen text, then confirm F1 and F9 still open their dedicated UMG tools.
- Known presentation decision: Matador Bull's orange `DrawDebug` line/box was its only directional warning. It is now hidden as requested. If the attack still needs a warning, implement a replicated production decal/Niagara telegraph rather than re-enabling DrawDebug.

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
Last synced: 2026-07-23T17:08:56+09:00

## Village Multi-Preset Handoff

- Uncommitted slot-capacity work adds `SlotSizeClass` Small (130m) / Medium (230m), with Medium as the backward-compatible default. Assignment rejects presets whose actual Footprint offset+extent exceeds capacity; overlap remains based on actual assigned Footprints. A separate orange editor-only `CapacityBounds` child avoids stripping the runtime root and cancels actor scale, while the inner Footprint stays unchanged. Undo and no-compatible debug paths are guarded. Full Editor build and `ProjectEden.Game.WorldLayout.VillageSelection` pass. Development Game verification is blocked by the unrelated installed `PCGExtendedToolkit` missing `PCGExCore.precompiled`. Choose which `Village_A..E` slots should be Small before saving map overrides.
- Village_01 Footprint centering is committed as `e4acdb19`; follow-up `35682b31` removes its accidental local `Y +2000cm` double correction. The same five authored layout actors moved together by `Y -2000cm`, `BP_CityAnchor` is at level XY `(0,0)`, and `PCGWorldActor0` remains at origin. Native and placed-Director Footprint offset stays `(0,0,-1500)` with the 130m extent. Rebuild Preview seed 186 in Top Orthographic confirms exact XY center alignment.
- Editor authoring follow-up: `AGP_VillageLayoutDirector` now exposes `Rebuild Preview` and `Clear Village Preview`. Rebuild uses `PreviewSeed`, loads transient non-saveable Level Instances at the exact runtime slot transforms, runs the existing instance tag/parameter setup, and optionally starts PCG sequentially in Preview editing mode. Runtime components remain Generate On Demand. Cleanup cancels PCG, unloads instances, destroys transient actors, and preserves the map's prior dirty state. Full Editor build and `ProjectEden.Game.WorldLayout.VillageSelection` pass.
- `bUsePreviewSeedInPIE` is an enabled-by-default Director debug switch. PIE now builds village selection from `PreviewSeed` before requiring GameState RunSeed, while non-PIE and packaged runtime continue using GameState RunSeed. The log prints both values when the override is active. Full Editor build and `ProjectEden.Game.WorldLayout.VillageSelection` pass.
- `L_LandscapeMap` has five unique SlotIds `Village_A..E`. The duplicated `GP_VillageSlot4/5` values were changed from `Village_C` to `Village_D/E` and the map was saved. Director `Rebuild Preview` at PreviewSeed 186 succeeded with `Village_A=Village_01` plus `Village_E=Village_00`; the log confirms two transient Level Instances and two sequential PCG components, with no duplicate-ID warnings.
- `AGP_VillageSlot::FootprintBounds` is a visualization-only flat XY preview at slot-local Z=0 (1cm half-height). This prevents the preview child from being snapped independently while the Director later spawns from the unchanged slot actor location.
- Mixed `L_Village_00`/`L_Village_01` selection is committed. The legacy fields define primary 00 with half extent `(11500,11500,3000)` cm (230x230m full XY); the additional preset pool contains compact 01 with centered offset `(0,0,-1500)` and half extent `(6500,6500,3000)` cm (130x130m full XY). The saved `L_LandscapeMap` Director overrides were updated to the same values. Duplicate IDs/paths are ignored and deterministic bounded assignment retries maximize the selected count before streaming.
- Final PIE: Seed `1372115260`, attempt 0, `Village_B=Village_01` and `Village_C=Village_00`; 01 isolated Road 3/District 1, 00 isolated Road 5/District 2, and sequential PCG completed `(2/2)`. Full Editor build and `ProjectEden.Game.WorldLayout.VillageSelection` automation pass.
- `Project_Eden/Content/WorldLayout/L_Village_01.umap` is tracked in the same commit as the source/tests. The editor is open on `L_LandscapeMap` with PIE stopped; the task did not save the already user-modified map or unrelated region/material/memoc changes.

## Village Footprint Handoff

- Source-only footprint work is complete. `AGP_VillageLayoutDirector` owns the current preset footprint; slots render it, red means overlap, and candidate conflict IDs feed deterministic non-overlap selection with required-group lookahead.
- Default half extent is `11500,11500,3000` cm and offset is `0,0,-1500` cm, so Village_00's visible XY footprint is 230m x 230m. Village_01 uses `6500,6500,3000` cm with centered offset `0,0,-1500`, so its visible XY footprint is 130m x 130m. Use `Refresh Footprint Preview` or `Rebuild Preview` after reopening the map.
- Full `Project_EdenEditor Win64 Development` build and `ProjectEden.Game.WorldLayout.VillageSelection` automation pass. Multi-preset support now uses each assigned preset's own footprint as described above.

## Village Layout Runtime V1 Handoff

- Source-only V1 is complete. Lobby `ServerTravel` appends `RunSeed`; `AGP_GameMode` owns the parsed/fallback value and `AGP_GameState` replicates it. `AGP_VillageLayoutDirector` is found or auto-spawned by GameMode and runs an authority-only deterministic policy over loaded `AGP_VillageSlot` actors.
- `AGP_VillageSlot` is non-spatial and excluded from Data Layers so candidate metadata is always available before World Partition selection. Give every placed slot a unique stable `SlotId`; use a shared `GroupId`, `SelectionWeight`, and matching Director `GroupRules`. Default native rule is optional `Village`, 50% chance, pick one.
- Current output is only `SelectedForRun`, selected slot IDs, debug boxes/labels, and logs. It deliberately does not spawn village content, activate Data Layers, call `GenerateLocal`, modify vegetation, or touch Region Event zones.
- Verified: full `Project_EdenEditor Win64 Development` build, `ProjectEden.Game.RunSeed.Flow`, and `ProjectEden.Game.WorldLayout.VillageSelection` pass. Editor Add search lists `GP Village Slot` and `GP Village Layout Director`. No map/content asset was saved.
- Next authored smoke test needs user-approved candidate positions/count. Recommended minimum: place three slots in one `Village` group, keep 50%/pick-one, run PIE repeatedly, and verify one-or-none debug selection changes with the logged RunSeed. V2 then maps each slot 1:1 to an initially Unloaded Runtime Data Layer and gates activation -> World Partition streaming completion -> city PCG completion -> vegetation activation/regeneration. Confirm `BP_CityAnchor` generate-on-load behavior before connecting this.

## PCG Vegetation Runtime Handoff

- `L_GameMap1` was verified at grass `GRID64`; the shared graph's current authored grass value is `GRID32`, while the default remains `GRID128`. Its previous Landscape Cache/`SurfaceSampler_1` errors are resolved.
- `L_LandscapeMap` is verified with Box extent `64000,64000,8000`: PIE generated 1,911 grass instances at start and 1,803 after a 400m move, with 50 old cells pooled. Cache mode is `SerializeOnlyAtCook` with 289 entries.
- `PCG_Vegetation_Global` is shared by several legacy maps. Their `BP_VegetationSpawner` instances may still inherit the Blueprint template defaults (`GenerateOnDemand`, non-partitioned, 32cm Box). Before expecting the same runtime behavior in another map, set that instance to Activated + GenerateAtRuntime + Partitioned and size its Box to the Landscape; then PIE-check bounded Surface Samplers there.
- Existing non-grass branches emit `Bounds Modifier` multiple-input warnings in PIE. They did not block grass generation; inspect that graph wiring separately if warning cleanup is requested.

## Landscape Region Boundary Blend Handoff

- Uncommitted macro-variation first pass: `M_StateMask` now inserts a default-false `UseRegionMacroVariation` branch immediately after `UseRegionVisualBlendV22` and before the optional slope/cliff overlay. It uses one shared world-XY sample from `T_RegionGround_MacroNoise_1024`, multiplies only BaseColor, and reconstructs/passes through the remaining fixed Material Attributes. `MI_RegionLandscape_GameMap2` alone overrides the switch true with `RegionMacroSizeMeters=180`, `RegionMacroStrength=0.16`, and the macro texture. The texture is saved linear (`sRGB=false`, Linear Color sampler). `Project_Eden/Scripts/Editor/ApplyRegionMacroVariation.py` safely reapplies/verifies this setup. Final commandlet returned exit 0 and logged `CODEX_REGION_MACRO_VERIFIED`; no SM6/material compile errors were found. Visual map inspection is the remaining step because Computer Use was accidentally cancelled while UE startup shaders were compiling.
- Candidate J plus variable blend width is approved and applied. Geometry uses 36-260m non-uniform PCHIP spacing, 4-26m random amplitudes, 50% sign persistence, 16m cap, and fixed junction/perimeter cores. Blend half-width varies independently per region-pair from 18-32px around 24px using 80-240m anchors; each region influence samples its own nearest boundary-side pixel. The runtime shader, hard RegionID, PCG, StateRT, and packed visual IDs are unchanged.
- Production validation passes 61/61 on two deterministic full runs. Observed width is 18.105-31.112px (mean 23.935px), actual gaps 81.045-237.979m, and unassigned boundary pixels, same-slot support/guard overlap, active-ID mismatch, ID-transition violations, topology changes, and protected-junction changes are zero. Weight PNG is `8209CBD4...11917`; IDs01/IDs23 remain `5FAD10BB...E0B7` / `7A0E2CBF...0913`. The prior Candidate J files are backed up under `Saved/CodexScratch/RegionVisualSlotsV22/candidate_j_before_random_width`; the external V2.2 mirror is updated byte-for-byte.
- V2.2 is the current GameMap2 Landscape path. `M_StateMask.UseRegionVisualBlendV22` defaults false; only `MI_RegionLandscape_GameMap2` saves true. Disable it for exact V2.1, then disable `UseRegionTransitionV21` for V2. PCG and the other five material instances remain unchanged/default false.
- New textures under `GameMap1_Smoothed/V2` are `T_GameMap1_RegionVisualIDs01V22` and `IDs23V22` (G8, Nearest, NoMip, Clamp, non-VT) plus `T_GameMap1_RegionVisualWeightsV22` (BGRA8/B8G8R8A8, Bilinear, NoMip, Clamp, non-VT). IDs are packed as low/high nibbles; no G16 or master precision override is used.
- The authoritative generator, PNGs, and report live in `Project_Eden/ArtSource/RegionVisualSlotsV22`; the external mirror is `VoronoIDTextureGen/GameMap1_RegionID_Smoothed/V2.2`. Generator SHA is `9EF39908...F14E`, report SHA `72EEA6EA...EF7`, and Weight PNG SHA `8209CBD4...11917`. Candidate J boundary seed remains 22037; variable-width seed is 83491. Junctions, segment endpoints, and the perimeter return to the 24px baseline.
- Computer Use reimported and saved `T_GameMap1_RegionVisualWeightsV22.uasset`; final size/hash are 717,075 bytes and `F91AB7E1...53CC6`. The texture is regular non-VT BGRA8, Bilinear, NoMipmaps/one mip, Clamp X/Y, sRGB off, and VectorDisplacementmap. A transient `DebugRandomRegionStates` + `RefreshRegionTexture` pass visibly applied the new weights. The user-owned map and MI hashes remain `E8CE0B5D...B728C` / `841EE208...D740`; neither was saved in this pass. Log shows a successful import/save and no related material, texture, or shader error after conversion.
- `MI_RegionLandscape_GameMap2.UseRegionVisualBlendV22` was rebuilt false/true after the final Weight import and saved true (29,152 bytes, hash `841EE208...D740`); 5/5 shaders compiled with no new material/sampler error. A later uniform beige editor view was not missing material: the Landscape used this MI, V2.2 and Weight were bound, but transient `RT_RegionState_15x1` was black. On placed `BP_RegionStateManager`, `DebugRandomRegionStates` then `RefreshRegionTexture` restored distinct regions immediately; use that sequence for non-persistent editor QA.
- The current `L_LandscapeMap.umap` working copy is modified relative to Git (10,397,897 bytes, `E8CE0B5D...B728C`, mtime 2026-07-20 01:18:26+09), predating the 02:26+ verification actions. Preserve it as user work. The validation state remains in memory and must not be saved merely to keep random QA colours.
- `L_LandscapeMap`, hard `T_GameMap1_RegionID`, StateRT, PCG, legacy Pair, and PairV2 were not modified by this warp. Current farm stripes, large state color differences, repeated four-state material art, road/PCG overlays, and steep outer-wall stretching are separate content/projection issues rather than V2.2 topology failures. A pre-apply binary backup exists in `Saved/CodexBackups/RegionVisualV22PreApply`.
- V2.1 now masks the residual pair-switch ridge and true multi-way junction failure. `M_StateMask` has one default-false `UseRegionTransitionV21` parameter feeding two static switches: new-vs-V2 Blend texture and neutral-vs-existing material attributes. Only `MI_RegionLandscape_GameMap2` saves V2.1=true; V2 and SeparateEdge remain true prerequisites. Disable that one bool for an immediate exact V2 rollback. PCG and other MIs were not touched.
- New assets are `MF_RS_TransitionNeutral`, `T_GameMap1_RegionBlendV21`, and `T_GameMap1_RegionTransitionV21` under the existing RegionSystem paths. The neutral function exposes unique BaseColor/Normal/ORM texture parameters. The auxiliary RG mask is Masks/Bilinear/NoMip/Clamp/non-VT; BlendV21 is Grayscale/Bilinear/NoMip/Clamp/non-VT. Both are 4096.
- Deterministic generation/validation is in `Saved/CodexScratch/RegionTransitionV21` and copied to external `VoronoIDTextureGen/GameMap1_RegionID_Smoothed/V2`. BlendV21 uses a 2-texel/~0.523m exact seam core and 24 texels/6.275m support per side (12.551m total), covers 6.634% versus old 33.742%, and gives ordinary same-pair seam canonical delta `0.0`. Hashes: Blend `55c337e9...90d`, RG mask `59c1bbeb...71d`.
- Saved `.copy` graph export confirms the shared bool feeds both V2.1 switches, R/G feed `Max -> Saturate -> NeutralBlend.Alpha`, the neutral result feeds both slope paths, and WPO stays on A. Lit/Unlit OFF/ON checks at pair-switch `(3713,-5204)` and junction `(5033,-4850)` show the black one-pixel stair line disappears and intersections become continuous. Shader compilation completed 165/165 with zero material errors. The current neutral dirt is intentionally provisional and appears bright/path-like, especially across the broad junction feather; tune/replace its texture parameters or regenerate narrower mask art later. Do not interpret that art caveat as a seam failure. `L_LandscapeMap` was not saved or modified.
- 2026-07-19 completed rollout: the long black/stepped seam was a shader graph contract bug, not texture resolution or filtering. `RegionID` had been changed to the authoritative owner, but the canonical-A state lookup still read that output. `MF_RS_GetRegionBlendData.MaterialExpressionAdd_0.A` now connects directly to `MaterialExpressionRound_0` and carries `CODEX_REGION_V2_CANONICAL_A_STATE_LOOKUP`. Re-exported graph text proves the saved edge; logs show 132/132 shaders completed without related errors.
- V2 assets remain under `/Game/RegionSystem/Textures/RegionID/GameMap1_Smoothed/V2`: PairV2 is Nearest/NoMip/Clamp/non-VT and BlendV2 is Bilinear/NoMip/Clamp/non-VT. Runtime contract is owner-relative: decode Pair `(A=min(P,N), B=max(P,N), OwnerIsB)`, reorder canonical states into Owner/Other, then blend `Owner -> Other` with `Mix=BlendByte/256` (`0..0.5`). Canonical-B weight is used only for continuity validation.
- `MI_RegionLandscape_GameMap2` is saved with `UseRegionBlendV2=true`, the two V2 textures, and `UseSlopeCliffOverlay=true` (`Start=.30`, `End=.60`, `WorldSizeUU=800`). Lit/Unlit close checks at World `(30244,-9322)` and a complex junction show the former long seam is gone. `L_LandscapeMap` was reloaded without saving and has no dirty marker. PCG and other MIs stay legacy.
- Generator/validation live in `Saved/CodexScratch/RegionPairV2Build` and were copied/run in external `VoronoIDTextureGen/GameMap1_RegionID_Smoothed/V2`. All 15 checks pass; deterministic hashes are PairV2 `748b2e8a...` and BlendV2 `5e1a3c97...`; 15 connected regions and 32 adjacencies are preserved, and ordinary seam derived-weight delta is `0.0`. Baseline V2 can retain a sub-meter (~0.52m) true-junction fallback; V2.1 now covers it with the neutral junction mask. Steep outer-wall XY projection stretching is a separate slope/geometry issue. No commit was made.

- `L_LandscapeMap` and `MI_RegionLandscape_GameMap2` use the new Pair-ID + smooth Edge texture path. `BP_RegionStateManager.ApplyLandscapeMaterial` now pushes `RegionEdgeTexture` as well as `IdTexture`.
- `M_StateMask` retains the exact legacy branch behind default-false `UseSeparateEdgeTexture`; the other five material instances remain off. The new blend function is `MF_RS_GetRegionBlendData`.
- PCG is intentionally not migrated: `PCG_Vegetation_Global` still overrides the legacy `T_GameMap1_RegionID`, has no Edge override, and its file hash stayed `45FC9A77...C3BA19`.
- Remaining 1m ownership stairs were caused by the preserved 1024 R contour, not Edge saturation. The external non-Git generator (`generate_from_existing_region_id.py`, SHA-256 `025D9A5A...`) now performs source-label SDF/top-2 reconstruction at 4096. Pair PNG SHA is `AD6DF2BE...`; Edge is `32B4F811...`. Validation preserved 15 connected regions, 32 adjacencies, aligned source samples, and area within 0.000071526pp; grid resolution is 26.147UU/texel. Both Unreal textures are 4096 and non-VT; Pair remains Nearest/NoMip and Edge remains Bilinear/mipped. Material recompile and PIE with Regions 0/1 forced to different states passed. Large V corners are real authored polygon vertices. Current repo changes are only the two texture uassets and are uncommitted.
=======
Last synced: 2026-07-24T07:01:06+09:00
>>>>>>> origin/main

## Regular Enemy Death Absorption Handoff

=======
Last synced: 2026-07-24T07:01:06+09:00

## Regular Enemy Death Absorption Handoff

>>>>>>> origin/main
=======
Last synced: 2026-07-24T07:01:06+09:00

## Regular Enemy Death Absorption Handoff

>>>>>>> origin/main
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

## Player HUD Restyle Handoff

- The B2 top-left HUD styling is saved but still needs PIE visual confirmation at 1280x720 and another DPI scale. The three attribute WBP assets share `T_UI_HUD_AttributeTrack_B` and `T_UI_HUD_AttributeFillMask_B`; `WBP_PlayerHUDWidget.TopLeftFrame` uses `T_UI_HUD_TopLeft_Backplate_B2` as `Draw As: Image`, `No Tile`. The editor reported successful compile/save for all four widgets.
- Source/import automation is `Project_Eden/Scripts/Editor/apply_player_status_hud.py`. It is idempotent and preserves the existing GAS bindings/widget names. `T_UI_HUD_TopLeft_Accent_B2` is imported but unused. If the background is too weak on bright terrain, tune the Backplate PNG/Brush tint after PIE rather than adding another opaque panel.
- Existing data caveat: `UGP_AttributeSet` has Health/MaxHealth and Mana/MaxMana but no Stamina pair; the Stamina WBP is likely a visual placeholder. Do not treat the successful visual restyle as proof that stamina gameplay data is implemented.

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

- Commits `10451155` and `6ffa4614` add `/Game/Effects/M_BossGroundHandTelegraph_Decal` and route the actual production BP CDO to it. The material uses `length(UV-0.5)` with radius `0.48`, edge softness `0.025`, pure-red `TelegraphColor`, and opacity `0.62`; it is independent of the Sweep fan material. The hand's `0.75s` warning, `115cm` radius, floor trace, reveal, collision, damage, and replication are unchanged.
- `Project_EdenEditor Win64 Development` passed. All 13 `ProjectEden.AI.Boss` tests passed, including the new production-BP material/path/domain/radial-mask/color contract and the existing Sweep decal regression. PIE-check flat and sloped floors for square corners, red readability, and all three decals per wave; no server test was run.
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

- 2026-07-26 Dark Knight/skill-range VFX cleanup: `Project_EdenEditor Win64 Development` built successfully. `ProjectEden.Combat` completed 19/19 with zero non-success results, including Dark Knight Charge/GroundCrack/DarkWave visual contracts, production telegraph override, and `SkillTargetPreview.UsesEmissiveDecal`. Branch commits are `1ec905b6`, `0b4e5d4d`, and `57af6e32`.
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

- 2026-07-26 PIE visual check remains for the Dark Knight Charge lane, Ground Crack radius/impact, Dark Wave orientation/scale, and player ground-skill range on both bright and very dark uneven terrain. No live multiplayer session was run because the user did not request one.
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

<<<<<<< HEAD
- 2026-07-24 Stage Zone work is code-complete but awaits content. User already set `Outer` Required/Pick=3 and `Middle` Required/Pick=4 on `L_LandscapeMap`. Reopen after the successful Editor build, add exactly one `AGP_EnemySpawnVolume` and one `APlayerStart` to every `L_Village_00..03`, keep streamed Zone Stage as Legacy because Director overrides it, and author each placed slot's landscape `RegionId`. Add persistent Center/Colosseum zones with explicit stages. Do not PIE-test until those actors exist; otherwise GameMode correctly logs that no zones were found.
- 2026-07-24 Zone-only navigation is implemented in GameMode. Unlocked stage Zones are invokers; completed/end-play Zones unregister. Defaults are generation `18000cm`, removal `22000cm`, retry `0.25s`, timeout `10s`. `DefaultEngine.ini` enables invoker-only generation and Dynamic Recast. The main map binary contains NavMeshBoundsVolume/RecastNavMesh. PIE still needs `P` visualization and runtime log validation after village Zone actors are authored.
- 2026-07-24 Outer→Middle travel-map foundation is implemented and uncommitted. Outer clear now spawns one persistent selection portal per Outer; overlap opens a client-only `UGP_MiddleTravelMapWidget` populated with current uncleared Middle zones. Marker selection RPC is server-validated by stage, ZoneId, incomplete state, and 600cm portal proximity, then teleports only that player to a same-LevelInstance actor tagged `MiddleArrivalAnchor` (Zone NavMesh fallback). Existing fixed Middle→Center→Colosseum portals remain unchanged. Editor build, Zone contracts, and minimap stability pass. Pending editor work: create `WBP_MiddleTravelMap`, assign it on `/Game/GAS_Pattern/Player/BP_GP_PlayerController`, and add `VillagePortalAnchor` plus `MiddleArrivalAnchor` TargetPoints to village presets.
=======
- PIE-check the simplified top-left HUD after removing `Vignette`, `CrestText`, `LocationTextBlock`, and `StatusHint`; verify the remaining frame height and padding do not leave excess empty space.
>>>>>>> origin/feature/no-mcp-work

- `ABP_UEFNSource_Player` EventGraph already does `Try Get Pawn Owner -> Cast BP_GP_PlayerCharacter -> Get CharacterTrajectory -> Set Character Trajectory`.
- `GP_CharacterAnimInstance` now reflects `CharacterTrajectory` off the owning character class instead of requiring a fragile nested property-access node in the AnimGraph.
- `Blend Poses by Enum` node added through tooling could not set protected `BoundEnum`; user rebuilt enum blend manually in editor.
- Root chooser rows now intentionally ignore `Walk`; MaskMan's first-pass custom chooser is `Idle / Run / Sprint / InAir`.
- For custom chooser authoring, use embedded nested choosers (`New Nested Chooser`) inside `CHT_MM_MaskMan_Root`; `Select Existing` only sees embedded choosers in that asset.

## Suggested Reads

Search first, then open only files named above.

### 2026-07-23 Village capacity handoff

- Capacity implementation and map assignment are committed as `817189ac`.
- Ten fixed seeds passed Small/Medium compatibility, conflict, and
  determinism checks. Standalone seeds 0/1/186 completed 7/7 PCG with
  nonzero managed ISM instances.
- Next, author a second Small preset (`L_Village_02`), then another Medium
  preset. Biome/region restrictions and Large capacity remain optional.

### 2026-07-22 Village streaming handoff

- Multi-instance isolation/sequential PCG is committed as `06017d09`; the saved map has three slots and fixed Required/PickCount=2.
- Range selection is limited to `GP_VillageTypes.h`, `GP_VillageSelectionPolicy.cpp`, and `VillageSelectionTests.cpp`. It adds opt-in deterministic `MinPickCount..MaxPickCount` while leaving fixed mode and saved-map behavior unchanged.
- Editor build and `ProjectEden.Game.WorldLayout.VillageSelection` / `ProjectEden.Game.RunSeed.Flow` passed on 2026-07-22 with the editor closed. Tests cover fixed compatibility, exact/ranged counts, determinism, candidate-order independence, validation, required/optional clamping, reflection, and defaults.
- To permit zero villages, keep the group optional and tune `SpawnChance`; do not set Min to zero. Next content task is adding more authored slots/presets and explicitly enabling range mode when desired.

### 2026-07-24 Village boss-phase handoff

- Village boss-phase source is complete and uncommitted. `AGP_EnemySpawnVolume::BossSpawns` is opt-in: after all normal markers and all registered normal/RegionEvent enemies are dead, GameMode spawns the boss composition and completes only after those bosses die.
- Normal box spawns prefer same-LevelInstance, in-box actors tagged `EnemySpawnPoint`; the boss prefers `BossSpawnPoint`. Tight NavMesh projection avoids rooftop snapping, while missing authored points retain safe fallbacks.
- Full `Project_EdenEditor Win64 Development` build/link succeeds. Author each `L_Village_00..03` once: several grounded TargetPoints tagged `EnemySpawnPoint`, one plaza TargetPoint tagged `BossSpawnPoint`, and one boss class/count in the Zone's `Boss Spawns`; `bIsBossZone` is not required.

### 2026-07-23 Radial skill selection handoff

- C++ implementation covers the eight-slot runtime wheel, center detail, Q/E click/key assignment, duplicate prevention with opposite-slot swap, K/Escape close, and local input blocking without world pause.
- UHT and all changed C++ translation units compiled successfully. The final build stopped only at `LNK1104` because the running editor owns `Binaries/Win64/UnrealEditor-Project_Eden.dll`.
- Seven final PNGs live under `Content/UI/Asset/SkillIcons/Radial/Source`; their exact importer/SkillData linker is `Scripts/Editor/import_radial_skill_icons.py`.
- 2026-07-24 screenshot review found the old authored screen still visible under the runtime wheel and entry labels spilling sideways. The fallback now collapses every existing host-canvas child before adding itself, and compact wheel entries hide their redundant label while the center detail retains the selected skill name.
- Compact wheel mode now replaces the authored button content with a dedicated centered `96x96` icon inside each `112x112` slot; the center-detail icon host is `128x128`.
- The importer has not run: `Content/UI/Asset/SkillIcons/Radial/Textures` and `[RadialSkillIcons]` log output are absent. Do not diagnose this as a runtime brush problem until the importer completes.
- Pending: stop PIE, execute the importer in the editor, confirm `[RadialSkillIcons] Completed successfully`, close the editor, run a normal build, then PIE-check K/Q/E, swap/move behavior, the inactive eighth slot, and 1280x720 plus another DPI scale.
- Do not stage the unrelated user-owned `Content/Asset/Nature/Materials/Bark_DeadTree_011.uasset`.

### 2026-07-24 Post-merge build repair

- `991d2230` manually introduced nested conflict blocks; `709efd9b`/`3a0ae4d3` removed markers but retained duplicate and malformed code. The repair preserves Stage Zones, village boss phase, three-player spawning, and the current intentional no-RegionEvent runtime contract.
- Removed duplicate enemy registrations/death-budget increments, repaired Dark Knight grants, restored Zone region fallback, removed stale deleted-runtime calls, and cleaned Config conflict markers.
- Full Editor build/link succeeds; `ProjectEden.Game.ZoneProgression.Contracts` and `ProjectEden.Combat.DarkArmorKnight.ProductionAbilityGrantContract` pass.
- Restored the three damaged LFS assets: Lobby/Map from valid `feature/vfx-skills` OIDs and FurnaceWalker from the valid pre-corruption `main` OID. `ProjectEden.Game.Lobby.LandscapeTravelConfiguration`, `ProjectEden.Game.LandscapeMap.Integrity`, and `ProjectEden.AI.Enemy.ProductionAnimationContract` all pass.

### 2026-07-26 Runtime bug-fix verification handoff

- Current uncommitted fixes cover explicit HP-zero life binding, death presentation/VFX, long-distance teammate spectating/recovery, ground-target cursor hiding, skeleton-safe forward roll, delayed enemy/boss spawn retries, and Colosseum animation/NavMesh softlocks.
- Elimination uses a one-time per-controller `GameOnly` lock. It no longer applies `UIOnly` or reclaims Slate focus on every countdown refresh, which previously blocked another same-process PIE viewport.
- Recovery now prioritizes the player's same-LevelInstance `PlayerStart` while their assigned Outer remains incomplete; completed Outer and later stages retain living-teammate recovery.
- Editor/Game/Server Development builds and three targeted automation contracts pass. A normal single-player PIE load also completes village and PCG generation.
- User verified the death presentation/recovery path, Client 1 roll, and Crystal Seraph portal order. Recheck input isolation in both directions and confirm a player killed in an incomplete Outer recovers in their own village.
- Equip a ground-target skill and verify only its decal appears while the OS/software pointer stays hidden; Q/E test slots were empty during the automated PIE attempt.
- Existing unrelated warnings remain: PCG Bounds Modifier receives multiple BoundsMin/BoundsMax items, PCGEx ResamplePath can receive fewer than two points, and Fab fence constructor assets are missing.
- Dedicated-server console support is uncommitted. `Project_EdenServer.Target.cs` builds `Project_EdenServer-Cmd.exe`; local and cooked launch BATs prefer it, keep a visible live-log CMD window, and print the actual server sandbox/package log file. Local verification loaded LobbyMap, listened on port 7778, and wrote `Saved/Cooked/WindowsServer/Project_Eden/Saved/Logs/Project_EdenServer.log`.

### 2026-07-26 Grounded enemy spawning

- Commit `04b98379` on `fix/grounded-enemy-spawns` removes the intermittent roof-spawn path. `EnemySpawnPoint`/marker scatter now stays on the ground anchor's reachable NavMesh island; authored failures return pending instead of using a broad SpawnBox query.
- Bosses retain the `800cm` projection needed by vertically offset Level Instances, but broad candidates must be path-connected to a tight zone ground anchor. Collision-adjusted capsule feet above the requested nav floor are rejected; partial boss batches roll back and retry.
- Verified: `Project_EdenEditor Win64 Development`, `ProjectEden.Game.EnemySpawnPlacement.GroundPolicy`, and `ProjectEden.Game.ZoneProgression.Contracts`.
- Not verified: a live/PIE multi-wave sweep across `L_Village_00..03`. Watch for intentional elevated combat platforms that may need a larger per-zone `MaxSpawnHeightAboveGroundAnchor`; do not disable the connected-nav requirement.

### 2026-07-26 Encounter spawn lifecycle

- Branch `fix/encounter-spawn-lifecycle` extends the grounded-spawn fix without changing configured enemy classes, counts, marker radii, recovery candidate order, or zone-completion ownership.
- Marker, zone-batch, staged-portal, and active-Colosseum safe-placement failures remain pending and retry. The configured timeout is now a one-time diagnostic threshold rather than a content-abandonment point.
- Boss summoned adds use connected ground alternatives and final-foot validation, then retire through normal enemy death presentation when the boss dies. They intentionally do not become zone objectives.
- Logout gate evaluation waits until the departing PlayerState has left `PlayerArray`; FinishRun clears retry callbacks before scheduling lobby return.
- Verified: `Project_EdenEditor Win64 Development` and full `ProjectEden` automation 69/69.
- Not verified: multiplayer PIE for a player already inside a marker when it activates, NavMesh delayed longer than 10 seconds, boss death with live adds, Center start after one waiting player disconnects, and active-Colosseum reconnect while NavMesh is rebuilding.
