---
memoc: true
type: state
scope: project-memory
updated: 2026-06-04T03:45:00+09:00
status: active
tags: [memoc, memoc/state]
---
# Session Summary
Last: 2026-06-04T05:22:00+09:00
Replace, do not append. Keep <800B.

## Status
- No UBT/build while editor open.
- Primary recovery embedded; sprint resumes after Primary.
- Crouch: C input, `bCanCrouch`, `CrouchedHalfHeight=64.f`, stance chooser routing.
- Mesh Z hacks removed.
- Retarget source-pin/sibling experiment reverted.
- Runtime ensures `UEFNSourceMesh` -> capsule and `CharacterMesh0` child.
- Source ABP PoseHistory MM node uses `RuntimePoseSearchDatabase`.
- Jump DB changes force MM interrupt once; normal DB changes keep smooth no-interrupt.

## Open
- PIE validate crouch PSD and jump transition speed.
