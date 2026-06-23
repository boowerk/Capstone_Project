---
memoc: true
type: worklog
scope: project-memory
created: 2026-06-09T09:44:07
updated: 2026-06-09T09:44:07
status: active
tags:
  - memoc
  - memoc/worklog
---
# fix dedicated server skill VFX lifecycle

actor: dyk66
actor_source: OS user
branch: unknown
status: done
created: 2026-06-09T09:44:07

## Summary

- Prevented predicted clients from replicating successful targeted-skill completion before the server executes gameplay.
- Preserved replicated cancellation while allowing the server projectile montage timer to spawn authoritative actors.
- Removed temporary diagnostic logs after successful dedicated-server verification.

## Changed Files

- `GP_TargetedSkillBase.cpp`
- `GP_Skill_NetTestProjectile.cpp`

## Verification

- `git diff --check` passed for both modified ability files.
- User confirmed dedicated-server skill VFX now work.

## Follow-up

- Build once after log cleanup, then commit.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/dyk66.md)
