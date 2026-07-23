---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-20T17:43:53
updated: 2026-07-20T17:43:53
status: active
tags:
  - memoc
  - memoc/worklog
---
# Stop passive target-facing turn

actor: lim
actor_source: git config user.name
branch: main
status: done
created: 2026-07-20T17:43:53

## Summary

- Keep target-acquisition turn as a one-time transition.
- Stop stationary Pawn Tick from repeatedly turning toward `TargetActor`.

## Changed Files

- `Project_Eden/Source/Project_Eden/Private/AI/Controllers/EnemyAIController.cpp`
- `Project_Eden/Source/Project_Eden/Private/Characters/GP_EnemyCharacter.cpp`

## Verification

- `Build.bat Project_EdenEditor Win64 Development ... -WaitMutex -NoHotReload` passed.

## Follow-up

_None._

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/lim.md)
