---
memoc: true
type: state
scope: project-memory
created: 2026-06-17T04:47:15
updated: 2026-06-18T13:22:06+09:00
status: active
tags: [memoc, memoc/state]
---
# Session Summary
Last: 2026-06-18T13:22:06+09:00
Replace, do not append. Keep <800B.

## Status
- Crystal Seraph uses shared `BT_BossCommon`/`BB_BossCommon` for patrol, chase, and reposition.
- GAS activation enforces a 2.5s shared cadence; prism/laser use last-use cooldown readiness.

## Verified
- Editor build and `ProjectEden.AI.Boss.PatternSelector.CrystalSeraph` passed.
- PIE cadence and flying common-BT movement remain unverified.

## Commits
- `1c9cd768`, `759988f2`, `e9616312`.

## Resume
- User-owned unstaged map/HUD/common-BT/minimap/memoc changes remain untouched.
