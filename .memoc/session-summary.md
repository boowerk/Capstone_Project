---
memoc: true
type: state
scope: project-memory
updated: 2026-06-20T22:20:00+09:00
status: active
---
# Session Summary
Last: 2026-06-20T22:20:00+09:00

## Status
- Regular enemies now inherit a screen-space `WorldHealthBarComponent` using `WBP_EnemyHealthBar` and GAS Health/MaxHealth.
- Bars show at full health, hide on death, and stay disabled for bosses using the HUD bar.
- User-owned `L_MainMap.umap` remains untouched/uncommitted.

## Verified
- Editor build passed.
- `ProjectEden.UI.EnemyHealthBar.Defaults` passed.

## Resume
- PIE-check bar height on melee, ranged, flying, and large meshes. Adjust the inherited component transform per Blueprint only when a nonstandard mesh needs it.
