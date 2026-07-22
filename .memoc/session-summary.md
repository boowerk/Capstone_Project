---
memoc: true
type: state
scope: project-memory
updated: 2026-07-22T19:40:00+09:00
status: active
tags:
  - memoc
  - memoc/state
created: 2026-07-18T13:21:02
---
# Session Summary

## Status
- Multi-village PCG isolation is committed (`06017d09`).
- Configurable village-count range is implemented and verified in the selection policy/types/tests.

## Behavior
- Legacy fixed `PickCount` remains default and the saved map still selects exactly 2 of its 3 slots.
- Optional range mode uses deterministic `MinPickCount..MaxPickCount`; required groups fail below Min, optional groups clamp, and zero villages remains `bRequired=false` plus `SpawnChance`.

## Verified
- Editor build succeeded with the editor closed.
- `VillageSelection` and `RunSeed.Flow` automation tests pass.
