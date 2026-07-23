---
memoc: true
type: state
scope: project-memory
updated: 2026-07-24T01:57:40+09:00
status: active
tags:
  - memoc
  - memoc/state
created: 2026-07-18T07:01:20
---
# Session Summary

- `c78e88a9`/`ba0f2bfa`/`29baec37` remove the fixed inward demo, objective marker, scripted Region Events, and automatic Dark Armor Knight/result path.
- Crystal Seraph prism shield remains body-centered; laser/VFX use the shield-surface hit. PIE remains pending.
- Party visual slot 1 (2P) is Stylized Paladin and slot 2 (3P) is Daelithra. Both reimported UE5-rigged meshes retain the MaskMan runtime Retarget AnimBP; 3-player PIE animation check remains.
- Party slot application now reapplies the MaskMan AnimationSet AnimBP after a mesh swap and assigns visual slots even when PlayerStart expansion falls back; Paladin's BP slot scale is `0.14`; Rider build passed.
- `EventMap.umap` preserves local Crystal Seraph work after the origin/main merge; audit it in editor for removed Region Event references.
