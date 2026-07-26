---
memoc: true
type: state
scope: project-memory
updated: 2026-07-26T20:32:39+09:00
status: active
tags:
  - memoc
  - memoc/state
created: 2026-07-24T01:26:27
---
# Session Summary

- Branch `fix/runtime-debug-visibility`, based on `fix/encounter-spawn-lifecycle`.
- Normal gameplay now disables transient engine on-screen messages through `DefaultEngine.ini` and `UGP_GameInstance`; direct C++ `AddOnScreenDebugMessage` calls are removed while ordinary `UE_LOG` diagnostics remain available in logs.
- Monster and skill `DrawDebug*` paths are behind the non-Shipping `g.DrawSkillDebug` opt-in, which defaults to `0`; Shipping always rejects the gate. Village selection drawing is separately double-opted-in and defaults off.
- F1 attribute debug UI and the F9 Encounter debug panel remain unchanged and available. Production decals, Niagara systems, preview actors, damage, overlap, replication, and timers remain on their original paths.
- Post-action motion-matching trajectory correction no longer depends on `bEnableDebugLog`; the flag now controls only its diagnostic log.
- Unused `DrawDebugLibrary` project activation and dead screen-message duration/offset settings were removed.
- `Project_EdenEditor Win64 Development` succeeds and all `ProjectEden` automation tests pass 70/70, including `ProjectEden.Debug.PresentationDefaults`.
- PIE still needs a visual pass confirming no monster/skill debug primitives or transient screen messages appear and F1/F9 remain usable.
- Matador Bull previously used an orange `DrawDebug` line/box as its only directional warning. It is intentionally hidden with other debug primitives; add a production decal/Niagara telegraph in a separate presentation ticket if that warning is still required.
