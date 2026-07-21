---
memoc: true
type: state
scope: project-memory
updated: 2026-07-21T22:24:01+09:00
status: active
tags:
  - memoc
  - memoc/state
created: 2026-07-18T07:01:20
---
# Session Summary

- `main` now includes enemy engagement fixes `bea85a35`, `a04c9d5e`, `e8dec768`, `788facec`.
- Attacks lock one player identity at activation, track that player's live position through prepare/windup, and lock direction at `AttackHit`; each activation applies one hit/spec.
- Melee uses physical ranges: Furnace step 350cm, Cyclops in-place 240cm. Cadence pursues outside 275cm, holds/faces smoothly inside 225cm, and resets range latch on player swap.
- Editor build and all 25 `ProjectEden.AI` tests pass. No live PIE/server run by request; protected assets unchanged.
