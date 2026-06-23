---
memoc: true
type: worklog
scope: project-memory
created: 2026-06-09T11:19:06
updated: 2026-06-09T11:19:06
status: active
tags:
  - memoc
  - memoc/worklog
---
# add Dark Stone ground launch skill

actor: dyk66
actor_source: OS user
branch: unknown
status: done
created: 2026-06-09T11:19:06

## Summary

- Added Dark Stone targeted ability with forward ground trace.
- Added replicated projectile that rises from below ground, then launches and explodes.
- Added dedicated Dark Stone cooldown tag.

## Changed Files

- `GP_Skill_DarkStone.h/.cpp`
- `GP_GroundLaunchProjectile.h/.cpp`
- `GP_Tags.h/.cpp`

## Verification

- Focused `git diff --check` passed.
- Build skipped per user request.

## Follow-up

- Create GA/BP/DA assets and test rise, launch, collision explosion, and server replication.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/dyk66.md)
