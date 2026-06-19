---
memoc: true
type: state
scope: project-memory
created: 2026-06-17T04:47:15
updated: 2026-06-18T00:27:00+09:00
status: active
tags: [memoc, memoc/state]
---
# Session Summary
Last: 2026-06-18T15:40:00+09:00
Replace, do not append. Keep <800B.

## Status
- Configured DragonSkull Control Rig in `/Game/Meshes/Monsters/DragonSkull`.
- Current rig asset found/updated: `CR_DeagonBone_SimpleJaw`; `CR_DeagonBone` is no longer present in the folder after user refresh/reimport.
- Controls: `global_ctrl > root_ctrl > body_offset_ctrl > head_ctrl`, plus `jaw_upper_ctrl` and `jaw_lower_ctrl`.
- Forward Solve graph maps `body_offset_ctrl -> root`, `head_ctrl -> Head`, `jaw_upper_ctrl -> jaw_upper`, `jaw_lower_ctrl -> jaw_lower` using global-space transforms.

## Changed
- Rebuilt `CR_DeagonBone_SimpleJaw` after asset normalization/reimport so controls are placed from current skeleton bone initial transforms.
- Fixed stale Control Rig offsets: controls are placed first in global space, then parented with maintain-global; graph drives translation/rotation only, never scale.
- Verified control global positions match `root`, `Head`, `jaw_upper`, and `jaw_lower`; all controls scale `1`.

## Resume
- Open `CR_DeagonBone_SimpleJaw`, move/rotate `body_offset_ctrl`, `head_ctrl`, `jaw_upper_ctrl`, and `jaw_lower_ctrl`; corresponding skull bones should follow. If the mesh still appears flipped in SkeletalMesh editor too, fix FBX axis/root rotation in DCC and reimport.
