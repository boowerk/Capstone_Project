---
memoc: true
type: worklog
scope: project-memory
created: 2026-05-29T06:33:26
updated: 2026-05-29T06:33:26
status: active
tags:
  - memoc
  - memoc/worklog
---
# fix idle short-walk motion matching start

actor: lim
actor_source: git config user.name
branch: main
status: done
created: 2026-05-29T06:33:26

## Summary

- Fixed idle-to-short-walk motion matching startup by treating grounded acceleration as movement intent before Speed2D passes the idle threshold.
- Restored current MM visibility by allowing the UEFNSource anim instance to print separate debug messages with DB/applied DB/selected anim.
- Left existing unrelated Blueprint/map/asset worktree changes untouched.

## Changed Files

- `.memoc/02-current-project-state.md`
- `.memoc/04-handoff.md`
- `.memoc/session-summary.md`
- `.memoc/worklog/lim/2026-05/20260529T0633-fix-idle-short-walk-motion-matching-start.md`
- `Project_Eden/Source/Project_Eden/Private/Animation/GP_CharacterAnimInstance.cpp`

## Verification

- Reviewed C++ diff; no local build run because user will verify through Live Coding.

## Follow-up

_None._

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/lim.md)
