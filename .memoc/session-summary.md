---
memoc: true
type: state
scope: project-memory
created: 2026-06-06T06:43:32
updated: 2026-06-10T00:00:00+09:00
status: active
tags:
  - memoc
  - memoc/state
---
# Session Summary
Last: 2026-06-12T00:00:00+09:00

## Status
- EventMap2 now has generated circular arena fence around Cylinder boundary using the intended Fence static mesh.

## Changed
- Replaced earlier generated fence ring with 48 `/Game/Fab/Model/Fence.Fence` StaticMeshActors.
- Actor labels `ArenaFence_000` through `ArenaFence_047`.
- Actor scale is `(2.16, 2.16, 2.16)` to overlap fence posts and close visual gaps.
- Placed on radius `5320.311680uu` around origin so arc spacing exactly matches the scaled Fence mesh length.
- Corrected rotation to Z yaw tangent alignment.
- Rebuilt arcade ring outside the fence: 48 arch bundles / 144 StaticMeshActors tagged `ArcadeRingGenerated`, matching the 48 fence segments.
- Arcade ring radius is `5729.577951uu`, actor scale is `(2.5,2.5,2.5)`, and arc spacing exactly matches the scaled P2/P3 arch span `750uu`.
- `ArcadeRing_Arch_NNN` uses `P2` and `ArcadeRing_Base_NNN` uses `P3`; P2/P3 pivots share exact location/rotation with the pillar pivot, and their local `-Y` span follows the chord to the previous pillar.
- Whole arcade ring is rotated `+3.75` degrees (`180/48`) relative to the fence so pillar pivots align to fence segment boundaries rather than fence mesh centers.
- `ArcadeRing_Pillar_NNN` uses `P1`, shares the same location as its `Arch/Base` bundle, and uses the radial/concentric yaw at that pillar; all generated arcade actor `Location.Z` values are `0`. With 48 bundles, `ArchYaw = PillarYaw - 3.75`.
- Moved generated plaza actors into level folders: 48 fence actors under `PLAZA_DE_TOROS/Fence`, 144 arcade actors under `PLAZA_DE_TOROS/Arcade`.
- Created `/Game/Fab/WorldAlignedMaterials/MI_WA_TerraCotta` from parent `/Game/Fab/M_WorldAlignedProjection`, using TerraCotta albedo/normal/roughness textures under `/Game/Textures/TerraCotta`.
- Added 48 `/Game/Fab/Model/Roof.Roof` StaticMeshActors labeled `ArcadeRing_Roof_000` through `ArcadeRing_Roof_047` at the current `ArcadeRing_Arch_NNN` transforms. Each roof matches the corresponding arch location, rotation, and scale exactly, and is placed in the same current arcade folder path.
- Current pillar-top stone ring intentionally keeps 16 `/Game/Fab/Model/Fence_Stone_Auditorium.Fence_Stone_Auditorium` StaticMeshActors: `ArcadeRing_Stone_001`, `004`, `007`, ... `046`. Each remaining stone uses scale `(2.5,2.5,2.5)`, keeps its existing Z, shares its pillar yaw, and was moved outward to radius `5854.577951uu` after an additional `80uu` horizontal outward offset.
- Added an outer concentric arcade ring: 48 bundles / 144 StaticMeshActors labeled `ArcadeRing_Outer_Pillar_NNN`, `ArcadeRing_Outer_Arch_NNN`, and `ArcadeRing_Outer_Base_NNN` under `PLAZA_DE_TOROS/Arcade_Outer`. The ring uses radius `6529.577951uu` (`+800uu` from the first-floor arcade), matches the first-floor arcade Z/rotation per index, keeps pillar scale `(2.5,2.5,2.5)`, and stretches arch/base local Y scale to `2.849066` to fit the larger chord.
- Added a second farther outer arcade ring: 48 bundles / 144 StaticMeshActors labeled `ArcadeRing_Outer2_Pillar_NNN`, `ArcadeRing_Outer2_Arch_NNN`, and `ArcadeRing_Outer2_Base_NNN` under `PLAZA_DE_TOROS/Arcade_Outer2`. The ring uses radius `7329.577951uu` (`+800uu` from `Arcade_Outer`), matches `Arcade_Outer` Z/rotation per index, keeps pillar scale `(2.5,2.5,2.5)`, and stretches arch/base local Y scale to `3.198132`.
- Added another arcade ring at explicit user values: 48 bundles / 144 StaticMeshActors labeled `ArcadeRing_Outer3_Pillar_NNN`, `ArcadeRing_Outer3_Arch_NNN`, and `ArcadeRing_Outer3_Base_NNN` under `PLAZA_DE_TOROS/Arcade_Outer3`. The ring uses radius `8055.0uu`, Z `2960.0uu`, first-floor arcade rotations/angles per index, and all P1/P2/P3 actors now use uniform scale `(3.514657,3.514657,3.514657)` so the high ring is enlarged overall rather than only stretched horizontally.
- Swapped the high `ArcadeRing_Outer3` arcade meshes from 1F to 2F variants: Pillar -> `/Game/Fab/Model/Arcade_2F_Arch_P1`, Arch -> `/Game/Fab/Model/Arcade_2F_Arch_P2`, Base -> `/Game/Fab/Model/Arcade_2F_Arch_P3`. The update touched 144 actors, preserving their existing transforms and uniform scale.
- Added 48 roof actors above the high arcade ring: `ArcadeRing_Outer3_Roof_000` through `ArcadeRing_Outer3_Roof_047` under `PLAZA_DE_TOROS/Arcade_Outer3_Roof`, using `/Game/Fab/Model/Roof.Roof`. Each roof copies the matching `ArcadeRing_Outer3_Arch_NNN` XY/rotation, uses uniform scale `(3.514657,3.514657,3.514657)`, and has its bounds bottom aligned to the matching high-arch bounds top. Sample verification passed for indices 0/12/24/36.

## Resume
- Manually inspect EventMap2 top view for arena/fence/arcade/roof/stone spacing and close-up inspect arch/roof/stone alignment. Generated rings use tags `ArenaFenceGenerated`, `ArcadeRingGenerated`, and `ArcadeStoneGenerated`.
