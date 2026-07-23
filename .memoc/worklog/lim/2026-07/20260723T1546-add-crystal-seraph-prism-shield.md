---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-23T15:46:00+09:00
updated: 2026-07-23T15:46:00+09:00
status: done
tags:
  - memoc
  - memoc/worklog
---
# Crystal Seraph prism shield

actor: lim
status: done

## Summary

- Added the translucent blue `M_CrystalSeraph_PrismShield` material with Fresnel rim, directional Seraph-facing boost, slow noise/pulse, fade, and release dissolve controls.
- Added `NS_CrystalSeraph_PrismShield` with `User.SeraphDirection`; the primary shield renderer remains the `BP_CrystalPrism` static mesh so no shield Actor/BP was added.
- `AGP_CrystalPrismActor` owns fade, directional material/Niagara input, and 0.3s release lifecycle.
- Root laser completion tells the boss to release its tracked prism set; reflected laser children intentionally do not do so.

## Verification

- Rider-style `Build.bat Project_EdenEditor Win64 Development ... -WaitMutex -FromMsBuild -architecture=x64`: passed.

## Follow-up

- PIE-check shell size/opacity and adjust BP shield scale or material scalar defaults if the Sculpture silhouette needs tighter framing.
