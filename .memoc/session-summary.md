---
memoc: true
type: state
scope: project-memory
updated: 2026-07-23T15:46:00+09:00
status: active
tags:
  - memoc
  - memoc/state
created: 2026-07-18T07:01:20
---
# Session Summary

- Crystal Seraph pattern BP children: Prism, Shard Projectile, Sanctuary Marker, and Seraph Laser; Wing Core mesh/collision are directly owned by `GP_CrystalSeraphBossCharacter`.
- `BP_Crystal_Seraph` references the three new pattern child classes; their mesh/Niagara values are BP defaults, not native hardcodes.
- BP component mesh overrides now win over C++ fallback mesh fields; Sculpture set on `BP_CrystalPrism.PrismMesh` will not become Cone at BeginPlay.
- Wing Core no longer spawns or tracks a child Actor: `GP_CrystalSeraphBossCharacter` owns its mesh/collision components directly; build passed.
- Laser Active/Reflection now reference dedicated Crystal Seraph Niagara copies; all six Attack2 User color inputs receive the material-like `#59ADFF` runtime tint.
- `BP_CrystalPrism.PrismVisualScale` is uniform `(2.9,2.9,2.9)` for Sculpture.
- Prism shield now has `M_CrystalSeraph_PrismShield`, `NS_CrystalSeraph_PrismShield`, BP mesh/material/VFX defaults, 0.2s fade-in, directional `User.SeraphDirection`, and 0.3s laser-end dissolve.
- Boss tracks active prisms only to release their owned shields when the root laser ends; reflected child lasers do not release them.
- Rider-style `Project_EdenEditor` build passed. `M_CrystalSeraph_RimFinal.uasset` and `EventMap.umap` remain user-owned and unstaged; PIE visual pass is pending.
