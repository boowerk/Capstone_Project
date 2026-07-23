---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-24T05:11:23+09:00
updated: 2026-07-24T05:11:23+09:00
status: active
tags:
  - memoc
  - memoc/worklog
---
# add enemy death absorption

actor: boowerk
actor_source: git config user.name
branch: main
status: done
created: 2026-07-24T05:11:23+09:00

## Summary

- Duplicated the simple skeletal dissolve into a production regular-enemy absorption system.
- Samples the actual dead enemy and attracts its particles to the authoritative killer's moving chest.
- Uses nearest living connected player as the three-player fallback and multicasts one stable target.
- Compresses the effect inside the corpse lifetime and protects the initial burst against frame hitches.

## Changed Files

- See commits `34c84570` and `3633f215`.
- `.memoc/02-current-project-state.md`
- `.memoc/03-decisions.md`
- `.memoc/04-handoff.md`
- `.memoc/session-summary.md`

## Verification

- `Project_EdenEditor Win64 Development` build passed.
- Absorption policy, production asset contract, and enemy death lifecycle tests passed.
- Compiled Niagara bindings/order were inspected; relevant Niagara warnings/errors were zero.
- Live server and PIE were intentionally not run.

## Follow-up

- PIE-check multiple basic enemy meshes while the killer moves and tune exposed component values if needed.
- Keep the regular enemy corpse lifetime at least two seconds while the effect remains attached.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/boowerk.md)
