---
memoc: true
type: state
scope: project-memory
updated: 2026-06-23T05:13:00+09:00
status: active
---
# Session Summary

## Status
- DarkArmorKnight: 49 Kwang sequences batch-retargeted via `RTG_KnightBoss` into `Boss/BP_Boss_DarkArmorKnight/Animations` with `A_DK_` prefix. ABP/AI not yet changed.
- DarkArmorKnight follow-up: created 10 `AM_DK_*` core pattern montages and clean 8-node `ABP_DarkArmorKnight` (Idle/Jog → DefaultSlot). Boss BP now uses ABP. New unbuilt Tag→Montage map + central native playback added; after user build, populate map CDO for Basic/Heavy/Sweep/Guard/Counter/Charge/DarkWave/GroundCrack/Groggy.
- DarkArmorKnight tuning: `A_DK_Jog_Fwd` PlayRate 0.6 and BP MaxWalkSpeed 180. Charge is now excluded from Ability-start montage playback; `GP_DarkKnightChargeActor::StartCharge` triggers tag montage. Added `AM_DK_ChargeLoop` from `A_DK_Sprint_Fwd`. Build/restart then map Charge tag to ChargeLoop.
- DarkArmorKnight build completed and `BP_DarkArmorKnight.PatternMontages` populated: Basic/Heavy/Sweep/Guard/Counter/Charge/DarkWave/GroundCrack/Groggy all map to corresponding `AM_DK_*` assets; Charge uses ChargeLoop.
- DarkArmorKnight latest unbuilt: retargeted `A_DK_UEFN_Sword_Dash_RM`; created `AM_DK_ChargeRM` and `AM_DK_DarkSlash`, mapped Charge/DarkWave to them. ChargeActor now uses montage root motion (duration 1.57s) instead of AddActorWorldOffset. DarkWave replaced projectile volley with 1/2 timed cone slashes. ABP root mode confirmed MontagesOnly. External build required.
- Seraph prisms are upright again and editable aura scale is reduced to 0.55 (`c8aac963`).
- User-owned editor config, map, and HUD assets remain uncommitted.
- Sans Sweep warning now uses `/Game/Effects/M_BossSweepTelegraph_Decal`, not debug lines.
- The dynamic red fan mirrors the real attack radius and angle, projects onto WorldStatic, and retains the 0.85s warning timing.
- Ground Hands warning and hit behavior remain unchanged.

## Verified
- `Project_EdenEditor Win64 Development` passed.
- `ProjectEden.AI.Boss.Sweep.UsesFloorDecal` passed.

## Resume
- PIE-check fan orientation and slope projection. No editor assignment is required.
- Preserve user changes in `DefaultEditor.ini`, `L_MainMap.umap`, and `WBP_PlayerHUDWidget.uasset`.
