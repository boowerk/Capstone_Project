---
memoc: true
type: worklog
scope: project-memory
created: 2026-05-31T10:02:50
updated: 2026-05-31T10:02:50
status: active
tags:
  - memoc
  - memoc/worklog
---
# wire radius multiplier to burst skills

actor: dyk66
actor_source: OS user
branch: unknown
status: done
created: 2026-05-31T10:02:50

## Summary

- Wired radius multiplier helper through SkillBase.
- Applied radius scaling to PulseBurst, GroundBurst, MineBurst, and ThrownBurst explosions.

## Changed Files

- `Project_Eden/Source/Project_Eden/Public/AbilitySystem/Abilities/GP_SkillBase.h`
- `Project_Eden/Source/Project_Eden/Private/AbilitySystem/Abilities/GP_SkillBase.cpp`
- `Project_Eden/Source/Project_Eden/Private/AbilitySystem/Abilities/Player/Character1/GP_Skill_PulseBurst.cpp`
- `Project_Eden/Source/Project_Eden/Private/AbilitySystem/Abilities/Player/Character1/GP_Skill_GroundBurst.cpp`
- `Project_Eden/Source/Project_Eden/Private/AbilitySystem/Abilities/Player/Character1/GP_Skill_MineBurst.cpp`
- `Project_Eden/Source/Project_Eden/Private/AbilitySystem/Abilities/Player/Character1/GP_Skill_ThrownBurst.cpp`
- `Project_Eden/Source/Project_Eden/Public/Actors/GP_AreaProjectile.h`
- `Project_Eden/Source/Project_Eden/Private/Actors/GP_AreaProjectile.cpp`
- `Project_Eden/Source/Project_Eden/Public/Actors/GP_MineBurstActor.h`
- `Project_Eden/Source/Project_Eden/Private/Actors/GP_MineBurstActor.cpp`

## Verification

- `git diff --check` passed; CRLF warnings only.
- Unreal build not run.

## Follow-up

_None._

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/dyk66.md)
