---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-18T06:41:40
updated: 2026-07-18T06:41:40
status: active
tags:
  - memoc
  - memoc/worklog
---
# Fix runtime PCG vegetation generation

actor: dyk66
actor_source: OS user
branch: unknown
status: done
created: 2026-07-18T06:41:40

## Summary

- Fixed Landscape-backed runtime PCG by changing the map cache from `NeverSerialize` to `SerializeOnlyAtCook` and expanding the vegetation actor bounds to the Landscape.
- Configured hierarchical vegetation grids (`GRID128` default, `GRID64` grass), bounded both Surface Samplers, and set cleanup hysteresis to `1.5`.

## Changed Files

- `Project_Eden/Content/Maps/MainMap/L_GameMap1.umap`
- `Project_Eden/Content/RegionSystem/PCG/PCG_Vegetation_Global.uasset`

## Verification

- PIE generated 7,135 grass instances on runtime partition actors with no PCG warning/error.
- Teleporting the pawn 400m replaced/pool-recycled old cells and generated new 64m/128m cells around the destination.

## Follow-up

- Other maps referencing the shared graph need their vegetation spawner runtime/partition/bounds overrides checked separately.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/dyk66.md)
