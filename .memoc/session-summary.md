---
memoc: true
type: state
scope: project-memory
updated: 2026-06-23T14:27:00+09:00
status: active
---
# Session Summary

## Status
- Boss target selection now flashes a shared `Boss Target Marker VFX` on the selected player's torso when `TargetActor` is first acquired or swapped.
- The component defaults to `/Game/Niagara/Vefects/Render_Particles_On_Top/VFX/Particles/NS_Render_Particles_On_Top_Stroke_03`.

## Verified
- `Project_EdenEditor Win64 Development` build passed.
- `ProjectEden.Combat.Boss.TargetMarkerVFXConfiguration` automation passed.

## Handoff
- Preserve user changes in `DefaultEditor.ini`, `L_MainMap.umap`, and `WBP_PlayerHUDWidget.uasset`.
- PIE-check target marker placement; tune socket/offset if `spine_03` is missing.
