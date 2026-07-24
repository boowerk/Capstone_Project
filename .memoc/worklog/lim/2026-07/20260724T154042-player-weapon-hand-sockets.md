---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-24T15:40:42+09:00
updated: 2026-07-24T15:40:42+09:00
status: complete
tags:
  - memoc
  - memoc/worklog
  - player
  - weapon
---
# Player weapon hand sockets

- Verified the saved MaskMan, Paladin, and Daelithra `hand_rSocket` assets; all are parented to deforming bone `hand_r`.
- Replaced the shared MaskMan C++ offset with `hand_rSocket` attachment and identity relative transform.
- Disabled absolute scale so each socket's authored scale is used; Paladin uses socket scale `4.2`.
- Extended `ProjectEden.Player.WeaponVisual.NiagaraSource` to verify attachment, identity transform, scale inheritance, and all three socket-parent contracts.
- Rider-equivalent `Project_EdenEditor` build and `ProjectEden.Player.WeaponVisual` 2/2 passed.
- Manual three-player PIE alignment remains; editor was closed normally for the build.
