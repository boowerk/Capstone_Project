---
memoc: true
type: worklog
scope: project-memory
created: 2026-06-28T15:10:00+09:00
updated: 2026-06-28T15:10:00+09:00
status: active
tags:
  - memoc
  - memoc/worklog
---
# add basic enemy cadence and footstep hearing

actor: boowerk
actor_source: git config user.name
branch: main
status: done
created: 2026-06-28T15:10:00+09:00

## Summary

- Replaced regular enemies' effective fixed three-second wait with per-archetype randomized attack cadence and initial group staggering.
- Added AI hearing perception and server-authoritative player footstep stimuli with walk/sprint/crouch tuning.

## Verification

- `Project_EdenEditor Win64 Development` build succeeded.
- `ProjectEden.AI.Enemy.AttackCadence` succeeded.
- `ProjectEden.AI.Perception.FootstepNoise` succeeded.

## Follow-up

- PIE-check encounter feel with mixed groups and hearing pursuit through a sight blocker.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/boowerk.md)
