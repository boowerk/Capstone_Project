---
memoc: true
type: state
scope: project-memory
status: active
tags:
  - memoc
  - memoc/state
updated: 2026-05-28T12:18:30
created: 2026-05-28T12:18:30
---
# Session Summary
Last: 2026-05-28T21:18:00+09:00
Replace, do not append. Keep <800B.
History: worklog. Resume risks: 04-handoff.md.

## Status
- Primary attack now stays on the character's current forward direction instead of snapping to current movement input.

## Changed
- `Project_Eden/Source/Project_Eden/Private/AbilitySystem/Abilities/Player/GP_Primary.cpp`: removed movement-input-based `SetActorRotation` before combo montage start.
- Removed now-unused `GP_PlayerController` include.

## Resume
- Build attempt blocked before compilation because Live Coding is active. Close editor/game or press Ctrl+Alt+F11, then rebuild.
