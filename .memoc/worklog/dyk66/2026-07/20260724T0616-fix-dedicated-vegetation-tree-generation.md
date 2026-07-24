---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-24T06:16:41
updated: 2026-07-24T15:38:00+09:00
status: active
tags:
  - memoc
  - memoc/worklog
---
# fix dedicated vegetation tree generation

actor: dyk66
actor_source: OS user
branch: unknown
status: done
created: 2026-07-24T06:16:41

## Summary

- Confirmed WindowsServer Cook includes `Availability=CPU` but strips texture platform pixels, so dedicated-server RegionID texture sampling cannot work.
- Corrected the tree-only data-loss flow around `Difference_0` while preserving village exclusion, the grass path, the Grid128/Grid64 hierarchy, and user graph tuning.
- Reworked the graph to avoid RegionID texture pixel sampling so it remains runnable on both clients and dedicated servers.
- Discarded the proposed `AGP_GameMode` server-disable path after packaged runtime verification succeeded with the texture-free graph.

## Changed Files

- `Project_Eden/Content/RegionSystem/PCG/PCG_Vegetation_Global.uasset`

## Verification

- Offline package check: the RegionID texture path is absent while the village exclusion, `Difference_0`, `Difference_1`, Grid128, and grass Grid64 data remain.
- `Build.bat Project_EdenServer Win64 Development ...` succeeded.
- User verified vegetation appears in the dedicated-server flow after the texture-free graph change.

## Follow-up

- Cook the PCG asset for every target that executes it; graph-only changes do not require a C++ build.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/dyk66.md)
