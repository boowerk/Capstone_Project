---
memoc: true
type: state
scope: project-memory
created: 2026-05-21T07:03:24
updated: 2026-06-13T00:00:00+09:00
status: active
tags:
  - memoc
  - memoc/state
---
# Current Project State

Last synced: 2026-06-13T00:00:00+09:00

## Current Status

- **Skill Augments & Pool**: Big Hammer (`DA_Augment_BigHammer_GiantImpact`) and Dark Solo (`DA_Augment_DarkSoloProjectile_VoidPierce`) are authored and registered in `DA_AugmentPool_SkillTest`.
- **Big Hammer C++ Skill**: Added as a ground-targeted falling strike. A replicated drop actor moves the hammer from a configurable height, spawns impact cues, and applies area hits.
- **Projectile Scaling & Pierce**: Projectile splash radius scales gameplay radius and Niagara parameters. Infinite pierce (`bProjectileInfinitePierce`) is supported (each enemy hit once, unlimited targets crossed). Spread angle is configurable per SkillData.
- **Lightning Strike Storm**: Supports periodic area damage via `AGP_PeriodicAreaDamageActor`. Strike radius and storm radius are scaled, and Niagara user parameters are mapped.
- **Niagara User-Parameter Overrides**: Replicated parameter overrides (Float, Int, Bool, Vec2D, Vec3, Color) are supported.
- **Ground-Targeted Foundation**: `UGP_TargetedSkillBase` supports ground position cursor preview, range clamping, trace, server revalidation, and selection modes (Instant, Projectile, Ray, TargetActor).
- **XP & Leveling**: Replicated progression (`CurrentXP`, `CurrentLevel`, etc.) with XP granted on enemy death. Added debugging output and a Blueprint event for level-up.
- **Augment Selection UI**: `UGP_AugmentSelectWidget` (C++ parent) rolls candidates from `UGP_SkillAugmentPoolData` without duplicate picks, using Game+UI input mode. Card backgrounds match type (Dawn/Dusk/Midnight/Zenith).
- **Multiplayer Movement & Inertia Fixes**: `AGP_PlayerController` sends local movement input to the server, resolving effective move input from acceleration to fix jitter and remote facing. Action inertia is restricted to standalone mode to prevent prediction correction jitter in multiplayer.
- **Boss Matador AI**: Boss Matador mage boss character (`AGP_MatadorMageBossCharacter`) and state component implemented, including decoy, chain, bull actors, and bull/groggy abilities. Bias toward range-keeping tactics.
- **Procedural Fence & Arena Layout**: Instanced-mesh procedural fence actor (`BP_Fab_FenceSegment`) and 48 generated arena fences placed symmetrically outside the EventMap2 boundary.
- **Concentric Arcade Rings**: 4-tier arcade rings (`ArcadeRing_Arch`, `ArcadeRing_Base`, `ArcadeRing_Pillar`) generated concentric to the arena at radial scales, including high 2F variant arches, bases, and matching roof rings.
- **Dusk/Aurora Lighting**: Preset balanced dusk lighting configuration defined for SkyLight, DirectionalLight, and PostProcess settings to achieve a "pretty dusk / blue aurora" aesthetic.
- **Motion Matching & Chooser Tables**: Custom root chooser `CHT_MM_MaskMan_Root` implemented for MaskMan locomotion (Idle, Run, Sprint, InAir). Sprint classification threshold tuned, and turn-in-place (TIP) logic updated to abort on movement input or acceleration.

## Project Snapshot

<!-- memoc:snapshot:start -->
- Last synced: 2026-06-09T08:33:26
- Detected stack: Not detected

### Source Directories

- `.claude`
- `Project_Eden`
- `skills`
<!-- memoc:snapshot:end -->

## Open Tasks

- Verify runtime behavior in PIE for skill augments and merged multiplayer movement fixes.
- Test Dark Solo Void Pierce against aligned enemies and wall collision.
- Connect `CHT_MM_MaskMan_Root` as the active chooser source and validate all locomotion states.
- Verify `UGP_AugmentSelectWidget` type-driven card background bindings after rebuild.

## Completed Tasks

- Fixed gitignore and local exclude files ignoring the `Augments/Skills/` folder recursively, and committed all missing Data Assets.
- Fixed dedicated-server skill execution predicted client cancellation.
- Replaced custom C++ alignment interpolation (`RInterpTo`) with motion matching root motion delta rotation for turn-in-place.
- Added `EGP_SkillAugmentType` widget mapping and card backgrounds.

## Commands

- Unreal Python: created `/Game/Characters/PlayerCharacter/ABP_UEFNSource_Player`
- Unreal BP edit: assigned `BP_GP_PlayerCharacter.UEFNSourceMesh.AnimClass = ABP_UEFNSource_Player`

## Notes

- Minimal source AnimGraph is `Motion Matching -> Pose History -> DefaultSlot -> Output Pose`.
- `GeneratedTrajectory` is connected to `Pose History.TransformTrajectory` in `ABP_UEFNSource_Player`.
- Action root motion uses retargeted speed ratio and fallback corrections.
