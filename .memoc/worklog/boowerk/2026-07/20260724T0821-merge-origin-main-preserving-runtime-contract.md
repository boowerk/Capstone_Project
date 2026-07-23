---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-24T08:21:17+09:00
updated: 2026-07-24T08:21:17+09:00
status: active
tags:
  - memoc
  - memoc/worklog
---
# merge origin main preserving runtime contract

actor: boowerk
actor_source: git config user.name
branch: main
status: done
created: 2026-07-24T08:21:17+09:00

## Summary

- Merged remote repair tip `e2bb3c31` without reviving removed corruption, random events, fixed-demo, or MiddleTravel runtime.
- Kept the verified local Landscape and lobby Blueprint LFS packages.
- Removed the incoming duplicate navigation section because the same dynamic-invoker settings already exist in the canonical config block.

## Verification

- Unmerged index entries and conflict markers: zero.
- Landscape and lobby packages match their LFS pointer size, SHA-256, and Unreal package magic.
- `Project_EdenEditor Win64 Development` build passed.
- Landscape integrity, three-player runtime starts, lobby Landscape travel, and enemy production-animation tests passed 4/4 under `NullRHI`.
- Live server and multiplayer PIE were intentionally not run.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/boowerk.md)
