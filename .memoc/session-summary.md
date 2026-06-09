---
memoc: true
type: state
scope: project-memory
status: active
tags: [memoc, memoc/state]
updated: 2026-06-06T00:00:00+09:00
---
# Session Summary
Last: 2026-06-06T00:00:00+09:00

## Status
- Projectile visuals now use explicit Projectile/Impact/Cast cue tags. SkillData supports optional direct-only or direct-plus-splash impact damage with radius and damage multiplier.
- MagmaShot uses skill-owned projectile/impact visuals: projectile Niagara comes from SkillData and projectile collision spawns the SkillData impact actor.
- `origin/main` ExpBar UI update (`d1afb656`) merged into `feature/vfx-skills`.
- Latest `origin/feature/vfx-skills-impact` merged into `feature/vfx-skills`; includes Primary VisualCues, Matador AI/BT updates, maps, and Fab assets.
- Selected augment visual overrides now work. Latest applicable `ActiveVFXOverride` / `ImpactVisualActorOverride` wins over SkillData element/default visuals.
- PlayerState owns override resolution; SkillBase consumes it for projectile and impact visuals.
- Augment `DamageMultiplier` now scales the full damage formula, including AttackPower/MagicPower coefficient contributions.
- Augment picker now excludes candidates whose `RequiredElementTag` does not match the player's current tech element.
- Existing XP/level, enemy XP reward, augment UI, duplicate prevention, numeric modifiers, and element requirements remain.

## Next
- Later: evolved skill-slot UI. Selected upgrade augment should override base skill name, description, and icon.
- Build.
- Create Pyros projectile augment targeting NetTestProjectile with `NS_Free_Magic_Projectile1`.
- Select augment in PIE and verify projectile VFX replacement.
- Verify 1.5 damage augment changes 10 damage to 15.

## Verify
- `git diff --check` passed. Full build not run.
