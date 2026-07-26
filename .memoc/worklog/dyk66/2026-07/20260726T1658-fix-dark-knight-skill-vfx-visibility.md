---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-26T16:58:04+09:00
updated: 2026-07-26T16:58:04+09:00
status: active
tags:
  - memoc
  - memoc/worklog
---
# Fix Dark Knight and skill-range VFX visibility

actor: dyk66
actor_source: OS user
branch: fix/dark-knight-skill-vfx-visibility
status: done
created: 2026-07-26T16:58:04+09:00

## Summary

- Added a soft circular deferred-decal material with an explicit Emissive output so combat ranges remain visible without relying on region lighting.
- Replaced Dark Knight Charge, Dark Wave, and Ground Crack engine Cube/Cylinder presentation with projected decals and sprite-only Niagara systems while preserving gameplay geometry.
- Overrode only the Dark Knight boss telegraph system, leaving the shared default used by other bosses unchanged.
- Applied the same emissive material to the player ground-target preview.

## Changed Files

- New material asset and repeatable Python generation script under `Content/Effects` and `Scripts/Editor`.
- Dark Knight Charge, Ground Crack, Dark Wave, boss character, and shared telegraph component source.
- Skill target preview source and focused automation contracts.

## Verification

- `Project_EdenEditor Win64 Development` build succeeded.
- `ProjectEden.Combat` automation completed 19/19 successfully.
- Tests assert the engine primitive components are absent, the selected Niagara paths are sprite-only alternatives, the production Dark Knight override survives Blueprint construction, and the player range material has a connected positive Emissive path.

## Follow-up

- PIE-check Charge lane alignment, Dark Wave orientation/scale, Ground Crack impact/radius, and player range readability on bright and very dark uneven terrain.
- No live multiplayer session was run; request one explicitly if replication/presentation timing needs manual proof.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/dyk66.md)
