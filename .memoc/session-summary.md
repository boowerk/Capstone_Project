---
memoc: true
type: state
scope: project-memory
updated: 2026-06-05T00:00:00+09:00
status: active
tags: [memoc, memoc/state]
---
# Session Summary
Last: 2026-06-05T00:00:00+09:00
Replace, do not append. Keep <800B.

## Status
- No UBT/build while editor open.
- Crouch: C input, `bCanCrouch`, `CrouchedHalfHeight=64.f`, stance chooser routing.
- Mesh Z hacks removed.
- Runtime UEFNSourceMesh -> capsule structure remains intact.
- Removed C++ RuntimePoseSearchDatabase path: ABP OnUpdate now directly evaluates Chooser and sets search DB.
- Cleaned up ApplyRuntimeDatabaseToMotionMatchingNode and ApplyChosenDatabase in GP_CharacterAnimInstance.
- Obsolete DB pin linking/binding code commented out in GP_AnimBlueprintEditorLibrary.
- Root motion extraction snaps inferred direction to 15deg by default and scales max sampled speed by 0.85 for constant mode.

## Open
- Validate Live Coding compilation and PIE tests (crouch PSD, jump response).
- Validate extracted RM assets in editor; refresh BP node pins for `FootPlantMaxSpeedScale` and `FootPlantDirectionSnapDegrees`.
