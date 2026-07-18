---
memoc: true
type: state
scope: project-memory
updated: 2026-07-18T21:24:21+09:00
status: active
tags:
  - memoc
  - memoc/state
created: 2026-07-18T07:01:20
---
# Session Summary

- AI/three-player slice is current through `0a22e69e`.
- Dark Knight now chases until Basic 350cm or Heavy 420cm; Dark Wave is capped to its real 520cm slash. Charge/GroundCrack remain ranged.
- GAS rechecks the committed target before activation, so stale BT state cannot start an out-of-range melee attack.
- Editor build, Dark Knight 6/6, and AI 21/21 pass. Do not run more live server tests unless asked.
- P0 manual: montage slot/contact, Dark Knight charge/timing; keep flying out until PIE.
- Preserve dirty `TestMap`, `DA_RegionEventData`, and `L_MainMap`.
