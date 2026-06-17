---
memoc: true
type: state
scope: project-memory
created: 2026-06-17T04:47:15
updated: 2026-06-18T05:59:00+09:00
status: active
tags: [memoc, memoc/state]
---
# Session Summary
Last: 2026-06-18T05:59:00+09:00
Replace, do not append. Keep <800B.

## Status
- Crystal Seraph teleport pattern and boss tactic restart fix are committed.
- Dedicated `BT_CrystalSeraph` uses `BB_EnemyCommon`, boss tactic service, guarded boss attack, and idle only.

## Verified
- Editor build and `ProjectEden.AI.Boss.PatternSelector.CrystalSeraph` passed.
- `L_MainMap` PIE initialized `BT_CrystalSeraph`/`BB_EnemyCommon`; no ground patrol fallback.

## Commits
- `3c012f46`, `4f4a978f`, `872454f3`, `7fe2f63e`.

## Resume
- User-owned unstaged map/HUD/common-BT/minimap/memoc changes remain untouched.
