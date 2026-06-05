---
memoc: true
type: state
scope: project-memory
updated: 2026-06-05T22:35:14+09:00
status: active
tags: [memoc, memoc/state]
---
# S 2026-06-05T22:35:14+09:00

## Status
- No UBT/build while editor open.
- Moving primary lower blends MM/montage: `MovingAttackLowerBodyMotionMatchBlendAlpha=0.65`.
- Primary idle-start full-body all; moving-start upper-only except 3rd/index2.
- Primary forces crouch, stops sprint, ignores crouch release until end, follows C hold.
- Camera idle/normal/sprint 340/380/460; socket `(0,65,20)`.
- Sword_Light VFX on UEFN Light A-D; crouch C/64/chooser.
- Mesh Z hacks removed; UEFNSourceMesh capsule intact.
- ABP OnUpdate Chooser->DB; obsolete DB pin code commented.
- RM extraction snap 15deg, scale 0.85.

## Open
- Live Coding/PIE validate crouch PSD, jump, RM; refresh BP pins.
