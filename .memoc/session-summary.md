---
memoc: true
type: state
scope: project-memory
created: 2026-06-01T04:41:15
updated: 2026-06-04T03:25:00+09:00
status: active
tags:
  - memoc
  - memoc/state
---
# Session Summary
Last: 2026-06-04T03:25:00+09:00
Replace, do not append. Keep <800B.

## Status
- No UBT/build while editor open.
- Primary recovery embedded; blocks sprint, resumes held sprint, logs `[PrimaryMove]`.
- `CHT_MM_MaskMan_Root_OriginalStyle` has Stance-based Crouch Idles/Walks routing.
- Removed unused MM LOD vars from `UGP_CharacterAnimInstance`.
- Added hold crouch: `IA_Crouch` -> C, controller Crouch/UnCrouch, player can crouch.
- Fixed chooser apply: crouch DB updates graph DB var + state.

## Open
- LiveCoding starts, no completion line yet.
- PIE validate crouch DB selection and Primary logs.
