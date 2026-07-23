---
memoc: true
type: state
scope: project-memory
updated: 2026-07-24T06:02:46+09:00
status: active
tags:
  - memoc
  - memoc/state
created: 2026-07-18T07:01:20
---
# Session Summary

- `9796240f`/`c971a3c2` tune regular-enemy death grains to fall before slowly curving into the latched player's chest.
- `SpriteSize=10` is preserved. Gravity is full through `.28s`, fades by `.60s`; attraction starts `.38s`, ramps `.80s to strength 800`, with drag `1.4`, playback `2.6x`, and stop `1.90s`.
- Editor build plus absorption policy/asset and death-lifecycle tests pass; Niagara warning/error/NaN scans are clean.
- Manual PIE visual tuning remains; no server/PIE run by request.
- The previously removed fixed demo/event flow remains removed.
- `c78e88a9`/`ba0f2bfa`/`29baec37` remove the fixed inward demo, objective marker, scripted Region Events, and automatic Dark Armor Knight/result path.
- Crystal Seraph prism shield remains body-centered; laser/VFX use the shield-surface hit. PIE remains pending.
- Party visual slot 1 (2P) is Stylized Paladin and slot 2 (3P) is Daelithra. Both reimported UE5-rigged meshes retain the MaskMan runtime Retarget AnimBP; 3-player PIE animation check remains.
- Party slot application now reapplies the MaskMan AnimationSet AnimBP after a mesh swap and assigns visual slots even when PlayerStart expansion falls back; Paladin's BP slot scale is `0.14`; Rider build passed.
- `EventMap.umap` preserves local Crystal Seraph work after the origin/main merge; audit it in editor for removed Region Event references.
