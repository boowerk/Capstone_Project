---
memoc: true
type: state
scope: project-memory
updated: 2026-07-24T05:11:23+09:00
status: active
tags:
  - memoc
  - memoc/state
created: 2026-07-18T07:01:20
---
# Session Summary

- `34c84570` adds `NS_EnemyDeath_Absorb`; `3633f215` connects regular-enemy deaths to it.
- The effect samples the dead enemy mesh, preserves its silhouette briefly, then follows the authoritative killer's live chest position. Nearest living player is the three-player fallback; bosses are excluded.
- Editor build plus absorption policy/asset and enemy-death lifecycle tests pass. Niagara warnings/errors: 0.
- Manual PIE visual tuning remains; no live server/PIE run by request.
- The previously removed fixed demo/event flow remains removed.
