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

- Crystal Seraph prism: prior full UBT passed; PIE pending. Shield is 250cm, body-centered; sphere ignores Visibility for foot IK and `PrismBodyCollision` blocks the crystal. Laser/VFX use the shield-surface hit. New reflection damage guard compiled but final DLL link is blocked by an active UnrealEditor process.
- Shield material: `Fresnel x DirectionMask x RimIntensity(3.0)`; EdgeSoftness `.10`, Falloff `2.5`.
- Enemy combat locks one target through windup and passes 25 AI tests.
- `M_CrystalSeraph_RimFinal.uasset` and `EventMap.umap` remain user-owned/uncommitted.
