---
memoc: true
type: wiki
scope: project-memory
created: 2026-06-17T00:00:00
updated: 2026-06-17T00:00:00
status: active
confidence: medium
tags:
  - memoc
  - memoc/wiki
  - memoc/topic
  - memoc/knowledge-wiki
  - pcg
  - gameplay-design
---
# PCG Region System & Gameplay Flow

How the project's PCG map system is structured and the candidate gameplay flows being designed on top of it. Confidence is medium: derived from code/asset/folder structure, not from `.uasset` internals (which are not text-readable). Needs editor/PIE verification on the marked items.

## PCG System Layout

Two PCG bodies exist:
1. `Content/PCG/` — early experimental/tutorial graphs (Biome, Building, Pathfinding tests). Not the main system.
2. `Content/RegionSystem/` — the real system: procedural city + nature generation plus a **Region State** layer.

### RegionSystem structure
- `PCG/CityGen/` — city generation from district/road splines into buildings (`PCG_CityGen`, `PCG_City_FromAnchor`, subgraphs for road-type checks, driveways, tree placement).
- `PCG/PCG_Vegetation_Global` — global vegetation scatter.
- `Blueprints/`:
  - `BP_CityAnchor` — anchor point that city generation builds from (enables per-region/district partitioning).
  - `BP_CityRoadSpline` / `BP_CityDistrictSpline` — road and district splines.
  - `BP_RegionSeed` — region seed.
  - `BP_RegionStateManager` — **region "state" manager (key for gameplay)**.
- `Data/Vegetation/` — three vegetation sets: **Alive / Corrupted / Dead**.

### Region State system (the gameplay hook)
- `BP_RegionStateManager` + `RT_RegionState_*` (render targets) + state textures (`State_00..03`).
- Vegetation has Alive/Corrupted/Dead variants; landscape material blends by state (`M_LandBlendState`, `MF_RS_GetRegionState`, `MF_RS_StateEqualsMask`).
- Net effect: **each region holds a state, and visuals (vegetation/terrain) change with it.** Runtime state transition is believed to work (per user) but is not yet verified in code.

### C++ connection points
- `APcgControllerActor` (`Source/.../PCG/PcgControllerActor.h`): injects PCG params via `ApplyPcgParameters`, re-captures minimap on generation finish (`NotifyPcgGenerationFinished`), and uses `UOpenAIRequester` to generate scatter params. Has `OnPcgLayoutReady` delegate.
- `FPCGScatterParams` (`PcgDataTypes.h`): scale/rotation ranges. `FPCGItemDetails` (`PCGStructs.h`): per-item mesh/weight/scale/rotation/offset. `PDA_Biome`, `PDA_PCGItemGroup` data assets.

### Maps
- Game default map = `RegionMap`; large test = `RegionMap_LargeTest` (+ a `_PCGSaveTest` baked variant, implying PCG output can be saved/baked rather than purely runtime-generated). Server default = `ServerTest`.

## Gameplay Flow (CONFIRMED 2026-06-17)

Core loop direction (see [[00-project-brief]]): skill-build + augment roguelike, boss rush, co-op. Element system is dropped.

Flow options considered: (A) single-arena wave defense, (B) wave defense then move to boss room, (C) procedural multi-room roguelike, (D) open field + boss gate, plus an earlier "shrinking zone / battle-royale storm" idea — **all superseded**.

### Confirmed direction: Linear City Progression
- **Core loop**: `[City] clear all designated enemies -> [Boss Room] kill boss -> [next City] -> ...` — a linear, designer-directed sequence of stages.
- **Enemy spawning**: only inside a specific designated City/District. Fixed enemy set per city. No around-player or continuous spawning. Players are funneled in a predetermined direction the team lays out.
- **Gate**: clearing all designated enemies in a city opens the route / teleports to the boss room; killing the boss advances to the next city.
- **No purification mechanic as a core loop.** Region State is demoted to **cosmetic feedback only**: when a city is cleared, the surrounding PCG vegetation changes slightly as a "result" of clearing it.
- **Boss room placement** (teleport vs reserved lot at city edge generated at runtime): both acceptable to the team; decision deferred. Teleport recommended for the slice (simplest).
- **Shrinking-zone / storm idea is dropped.**

### NavMesh note for this design
- NavMesh is NOT produced by PCG. The navigation system builds it from collision geometry inside a `NavMeshBoundsVolume`; the landscape provides the walkable floor and PCG-spawned props become obstacles.
- Because only specific cities are combat zones (no runtime terrain change during combat), **Static navmesh generation is fine** — just cover each city with a `NavMeshBoundsVolume`. (Runtime Dynamic + Navigation Invokers would only be needed if PCG reshaped terrain mid-combat or for around-player spawning, neither of which this design uses.)
- Spawn placement must still project candidate points onto the navmesh (`ProjectPointToNavigation` / `GetRandomReachablePointInRadius`) so enemies never spawn off-mesh. Do NOT reuse raw PCG vegetation points (no navmesh guarantee, bad placement quality).

## Needs verification (editor/PIE, `.uasset` internals)
1. Does `BP_RegionStateManager` expose a **runtime** state-transition API (vs editor-setup only)?
2. Is PCG **runtime-generated** or **baked/saved** (the `_PCGSaveTest` map suggests baked output exists) — affects co-op server/client sync.
3. Can spawn areas be partitioned per District for enemy/zone control?

## Related
- [Wiki Index](../../index.md), [Topics](README.md)
- [[00-project-brief]] — gameplay direction
- [[02-current-project-state]] — active systems
