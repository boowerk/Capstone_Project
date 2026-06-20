---
memoc: true
type: state
scope: project-memory
created: 2026-06-17T04:47:15
updated: 2026-06-19T08:31:17+09:00
status: active
tags: [memoc, memoc/state]
---
# Session Summary
Last: 2026-06-19T08:31:17+09:00
Replace, do not append. Keep <800B.

## Status
- Sans Ground Hands runs 3 staggered hands x 3 waves with a 0.75s red decal; hits deal damage and launch upward.
- Sans summon now uses `Basic/BP_BasicEnemy_Melee` instead of the removed legacy asset.

## Verified
- Full editor build and Sans Ground Hands selector test passed.
- PIE decal, timing, collision, and launch remain unverified.

## Commits
- `f4e75d83`, `8a7fbc60`, `e080f16c`.

## Resume
- User-owned MainMap and untracked Matador memo remain untouched.
