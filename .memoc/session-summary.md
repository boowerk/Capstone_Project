---
memoc: true
type: state
scope: project-memory
created: 2026-05-26T11:35:43
updated: 2026-05-28T20:13:00
status: active
tags:
  - memoc
  - memoc/state
---
# Session Summary
Last: 2026-05-28T20:13:00
Replace, do not append. Keep <800B.
History: worklog. Resume risks: 04-handoff.md.

## Status
- Tech element flow works: PlayerState tech tag drives skill VFX and elemental damage/resistance.
- `GP_TechSelectWidget` exists; test WBP/buttons work. Test controller opens it with raw `K`.
- Added DA actor/VFX path: SkillData now has `SpawnActorClass`; element entries have `ProjectileVisualSystem`.

## Changed
- Projectile/AreaProjectile replicate `ProjectileVisualSystem` and expose `BP_OnProjectileVisualSystemChanged`.
- ThrownBurst/MineBurst/NetTest/SplitShot prefer DA `SpawnActorClass`, then fallback to old GA class vars.
- ThrownBurst/NetTest/SplitShot pass DA projectile VFX. ThrownBurst/MineBurst pass element impact actor.

## Open Tasks
- Rebuild in editor/VS, fill DA `SpawnActorClass`/projectile VFX, implement BP event to set Niagara asset.
- Later replace raw `K` with Enhanced Input `IA_ToggleTechSelect`.
