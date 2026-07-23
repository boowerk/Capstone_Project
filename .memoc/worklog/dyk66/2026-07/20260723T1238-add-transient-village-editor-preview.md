---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-23T12:38:00+09:00
updated: 2026-07-23T12:38:00+09:00
status: active
tags:
  - memoc
  - memoc/worklog
---
# Add transient village editor preview

actor: dyk66
actor_source: OS user
branch: feature/vfx-skills
status: done
created: 2026-07-23T12:38:00+09:00

## Summary

- Added Director buttons to rebuild or clear non-saveable village Level Instance previews.
- Reused runtime selection, per-instance tag isolation, graph overrides, seeds, transforms, and sequential PCG ordering.
- Kept production PCG Generate On Demand; editor generation uses PCG Preview mode.

## Changed Files

- `Project_Eden/Source/Project_Eden/Public/Game/WorldLayout/GP_VillageLayoutDirector.h`
- `Project_Eden/Source/Project_Eden/Private/Game/WorldLayout/GP_VillageLayoutDirector.cpp`
- `Project_Eden/Source/Project_Eden/Private/Tests/VillageSelectionTests.cpp`

## Verification

- Full `Project_EdenEditor Win64 Development` build succeeded.
- `ProjectEden.Game.WorldLayout.VillageSelection` passed.
- Editor buttons appeared and invoked selection. Current map data selected no slots because `Village_C` is duplicated three times.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/dyk66.md)
