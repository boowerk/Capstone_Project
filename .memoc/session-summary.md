---
memoc: true
type: state
scope: project-memory
updated: 2026-06-21T06:50:00+09:00
status: active
---
# Session Summary
Last: 2026-06-21T06:50:00+09:00

## Status
- Anchor leash finalized in `93ac0b15` + `177bc9ad`.
- Crossing `ReturnHomeDistance` starts return even with a target.
- Inside the default 75% inner boundary, a visible player interrupts return and resumes combat; hysteresis prevents boundary stop/oscillation.
- User-owned `L_MainMap.umap` remains untouched/uncommitted.

## Verified
- Editor build passed.
- `ProjectEden.AI.Enemy.LeashPolicy` passed.

## Resume
- PIE-check melee/ranged/flying crossing the outer anchor and re-engaging inside the inner boundary.
