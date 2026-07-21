---
memoc: true
type: state
scope: project-memory
updated: 2026-07-21T17:32:45+09:00
status: active
tags:
  - memoc
  - memoc/state
created: 2026-07-18T13:21:02
---
# Session Summary

## Status
- Single-preset village streaming works: `L_Village_00` loads at one deterministic selected slot and generates PCG after load/show.

## Verified
- Editor build and `RunSeed.Flow` / `VillageSelection` pass.
- PIE seed `993821918` selected `Village_B`; one PCG component configured/generated, and the user visually confirmed the village at the slot.

## Resume
- Keep the fixed main-map village instance removed.
- Multiple villages need per-instance Road/District tags plus PCG parameter overrides. Client sync and cook registration remain.
