---
memoc: true
type: state
scope: project-memory
updated: 2026-06-21T06:30:00+09:00
status: active
---
# Session Summary
Last: 2026-06-21T06:30:00+09:00

## Status
- Enemy leash fix completed in `aa0522f5` + `971d8f29`.
- A valid target prevents return-home; target loss outside `ReturnHomeDistance` starts return; seeing the player again interrupts return.
- User-owned `L_MainMap.umap` remains untouched/uncommitted.

## Verified
- Editor build passed.
- `ProjectEden.AI.Enemy.LeashPolicy` passed.

## Resume
- PIE-check melee/ranged/flying chase beyond their old leash, target loss outside it, and player re-entry during return.
