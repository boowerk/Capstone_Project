---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-26T20:32:39+09:00
updated: 2026-07-26T20:32:39+09:00
status: active
tags:
  - memoc
  - memoc/worklog
---
# Runtime debug visibility

actor: dyk66
actor_source: OS user
branch: fix/runtime-debug-visibility
status: done
created: 2026-07-26T20:32:39+09:00

## Summary

- Disabled transient engine on-screen messages while preserving F1/F9 dedicated debug UIs.
- Put monster and skill world debug drawing behind a default-off non-Shipping opt-in without changing production decals, Niagara, damage, targeting, replication, or timers.
- Removed direct C++ screen-message calls, the unused DrawDebugLibrary activation, and dead message-presentation settings.
- Separated post-action trajectory correction from its diagnostic log flag.

## Verification

- `Project_EdenEditor Win64 Development` succeeded after final changes.
- `ProjectEden.Debug.PresentationDefaults` passed.
- Full `ProjectEden` automation passed 70/70 with exit code 0.
- Direct C++ `AddOnScreenDebugMessage`/`PrintString` search returned no matches.
- `git diff --check` and `.uproject` JSON parsing passed.

## Follow-up

- PIE-check representative player skills and boss attacks for absent debug primitives and screen text.
- Verify F1 and F9 still open the existing dedicated UMG tools.
- Decide whether Matador Bull needs a production decal/Niagara direction warning to replace its former debug line/box.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/dyk66.md)
