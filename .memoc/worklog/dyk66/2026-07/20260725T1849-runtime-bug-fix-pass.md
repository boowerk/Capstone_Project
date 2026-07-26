---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-25T18:49:25
updated: 2026-07-26T12:42:00+09:00
status: active
tags:
  - memoc
  - memoc/worklog
---
# Runtime bug-fix pass

actor: dyk66
actor_source: OS user
branch: refactor/codebase-cleanup
status: done
created: 2026-07-25T18:49:25

## Summary

- Fixed player elimination VFX/presentation, authoritative teammate spectating, hidden ground-target cursor, and production root-motion roll.
- Kept elimination input locking local to the dead controller without `UIOnly` or repeated Slate focus changes, so another same-process PIE client remains controllable.
- Kept an eliminated player in their assigned village while its Outer is incomplete by recovering at that Level Instance's existing `PlayerStart`.
- Removed the stale Blueprint-delegate dependency that left players mobile at zero HP by binding life state directly after ASC avatar initialization.
- Made forward roll skeleton-aware for alternate party meshes and removed input-derived client/server re-rotation.
- Added pending/retry accounting for delayed zone spawns and a single per-zone NavMesh retry chain.
- Added safe boss-point fallback and retry gating so Crystal Seraph cannot be skipped into a portal when NavMesh projection fails.
- Reduced Colosseum build cost and removed its fixed NavMesh timeout softlock.
- Added a dedicated-server console sibling and launcher handling so live server logs are visible while preserving the normal deployment executable.

## Changed Files

- Player character/controller source and two production-contract tests.
- GameMode zone progression/runtime state source and contract tests.
- MaskMan animation-set and Colosseum animator Blueprint assets.
- Server target plus local/cooked dedicated-server launch scripts and documentation.

## Verification

- `Project_EdenEditor`, `Project_Eden`, and `Project_EdenServer` Win64 Development builds succeeded.
- Zone progression, player life-state, and production roll automation tests succeeded.
- Single-player PIE loaded selected village and PCG content without a new crash.
- `Project_EdenServer-Cmd.exe` built successfully; the local launcher streamed live output, listened on port 7778, and wrote the expected sandbox log.

## Follow-up

- User verified death/spectate/recovery/VFX, Client 1 forward roll, Middle_01 Seraph-before-portal, and incomplete-Outer recovery. Recheck two-client input isolation and an equipped ground-skill decal.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/dyk66.md)
