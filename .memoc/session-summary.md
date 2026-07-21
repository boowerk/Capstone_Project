---
memoc: true
type: state
scope: project-memory
updated: 2026-07-21T21:15:00+09:00
status: active
tags:
  - memoc
  - memoc/state
created: 2026-07-18T07:01:20
---
# Session Summary

- `main` includes seeded inward demo flow and enemy transition commits `1ce0f79d`, `9e7dfeaf`.
- Three players run outer Rift -> Defense -> Shrine -> center Dark Knight with seed-varied locations and 2-of-3 quorum.
- Regular enemies now run Face -> AttackPrepare -> full GAS montage -> Recovery -> ChaseResume; temporary bridges use each DataAsset's Idle and are replaceable.
- ActionEnd no longer stops the montage tail; commit spans the whole sequence and the phase replicates for clients.
- Editor build and all 23 `ProjectEden.AI` tests pass. No live server/PIE run; protected assets unchanged.
